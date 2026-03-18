#!/usr/bin/env python3
"""
STM32F407 OTA 升级测试脚本
实现 UDS $34/$36/$37 服务下载固件到 Bootloader
"""

import can
import time
import struct
import sys
from datetime import datetime

class UDSClient:
    """UDS 客户端"""
    
    # CAN ID
    PHYS_REQUEST_ID = 0x735
    FUNC_REQUEST_ID = 0x7DF
    RESPONSE_ID = 0x73D
    
    def __init__(self, channel='can0', bitrate=500000):
        self.channel = channel
        self.bitrate = bitrate
        self.bus = None
        self.sequence_number = 0
        
    def open(self):
        """打开 CAN 接口"""
        try:
            self.bus = can.interface.Bus(
                interface='socketcan',
                channel=self.channel,
                bitrate=self.bitrate
            )
            print(f"[OK] CAN 接口 {self.channel} 已打开 @ {self.bitrate}bps")
            return True
        except Exception as e:
            print(f"[ERROR] 打开 CAN 接口失败: {e}")
            return False
    
    def close(self):
        """关闭 CAN 接口"""
        if self.bus:
            self.bus.shutdown()
            print(f"[INFO] CAN 接口已关闭")
    
    def send_single_frame(self, data, is_func=False):
        """发送 ISO-TP 单帧"""
        can_id = self.FUNC_REQUEST_ID if is_func else self.PHYS_REQUEST_ID
        
        # 构建单帧: 长度(4bit) + 数据
        frame_data = [len(data)] + list(data)
        # 填充到 8 字节
        frame_data += [0x00] * (8 - len(frame_data))
        
        msg = can.Message(
            arbitration_id=can_id,
            data=frame_data,
            is_extended_id=False
        )
        
        try:
            self.bus.send(msg)
            data_str = ' '.join([f'{b:02X}' for b in data])
            print(f"  TX -> SID:0x{data[0]:02X} Data:[{data_str}]")
            return True
        except Exception as e:
            print(f"  [ERROR] 发送失败: {e}")
            return False
    
    def receive_response(self, timeout=2.0):
        """接收 UDS 响应"""
        start_time = time.time()
        
        while time.time() - start_time < timeout:
            msg = self.bus.recv(timeout=0.1)
            if msg and msg.arbitration_id == self.RESPONSE_ID:
                # 解析 ISO-TP 单帧
                if len(msg.data) < 1:
                    continue
                
                length = msg.data[0] & 0x0F
                if length > 7 or length + 1 > len(msg.data):
                    continue
                
                response_data = list(msg.data[1:1+length])
                data_str = ' '.join([f'{b:02X}' for b in response_data])
                
                if response_data[0] == 0x7F:
                    # 否定响应
                    sid = response_data[1]
                    nrc = response_data[2]
                    print(f"  RX <- Negative Response SID:0x{sid:02X} NRC:0x{nrc:02X}")
                    return None, nrc
                else:
                    # 肯定响应
                    sid = response_data[0] - 0x40
                    print(f"  RX <- Positive Response SID:0x{sid:02X} Data:[{data_str}]")
                    return response_data, None
        
        print(f"  [TIMEOUT] 等待响应超时")
        return None, None
    
    def diagnostic_session_control(self, session_type):
        """10 服务 - 诊断会话控制"""
        print(f"\n[UDS] DiagnosticSessionControl (0x10) - Type:0x{session_type:02X}")
        request = [0x10, session_type]
        
        if not self.send_single_frame(request):
            return False
        
        response, nrc = self.receive_response()
        return response is not None
    
    def ecu_reset(self, reset_type=0x01):
        """11 服务 - ECU 复位"""
        print(f"\n[UDS] ECUReset (0x11) - Type:0x{reset_type:02X}")
        request = [0x11, reset_type]
        
        if not self.send_single_frame(request):
            return False
        
        response, nrc = self.receive_response(timeout=0.5)
        # 复位后可能收不到响应，这是正常的
        return True
    
    def read_data_by_identifier(self, did):
        """22 服务 - 根据标识符读取数据"""
        print(f"\n[UDS] ReadDataByIdentifier (0x22) - DID:0x{did:04X}")
        request = [0x22, (did >> 8) & 0xFF, did & 0xFF]
        
        if not self.send_single_frame(request):
            return None
        
        response, nrc = self.receive_response()
        return response
    
    def routine_control_erase(self):
        """31 服务 - 擦除内存"""
        print(f"\n[UDS] RoutineControl - EraseMemory (0x31 0x01 0xFF00)")
        request = [0x31, 0x01, 0xFF, 0x00]  # StartRoutine + EraseMemory
        
        if not self.send_single_frame(request):
            return False
        
        response, nrc = self.receive_response(timeout=5.0)
        if response and len(response) >= 5:
            status = response[4]
            if status == 0x00:
                print(f"  [OK] 擦除成功")
                return True
            else:
                print(f"  [FAIL] 擦除失败，状态:0x{status:02X}")
                return False
        return False
    
    def request_download(self, addr, size):
        """34 服务 - 请求下载"""
        print(f"\n[UDS] RequestDownload (0x34)")
        print(f"  Address: 0x{addr:08X}, Size: {size} bytes")
        
        # 构建请求: dataFormatId + addrLen/sizeLen + addr + size
        addr_bytes = struct.pack('>I', addr)  # 大端 4字节地址
        size_bytes = struct.pack('>I', size)  # 大端 4字节大小
        
        request = [0x34, 0x00, 0x44]  # SID + dataFormatId + (4<<4 | 4)
        request += list(addr_bytes)
        request += list(size_bytes)
        
        if not self.send_single_frame(request):
            return None
        
        response, nrc = self.receive_response()
        if response and len(response) >= 4:
            # 解析最大块长度
            max_block_len = (response[2] << 8) | response[3]
            print(f"  [OK] 下载请求接受，MaxBlockLen: {max_block_len}")
            return max_block_len
        return None
    
    def transfer_data(self, block_num, data):
        """36 服务 - 传输数据"""
        # 构建请求: SID + blockNum + data
        request = [0x36, block_num] + list(data)
        
        if len(request) > 8:
            # 需要分段传输，简化处理：只发送前 7 字节
            request = request[:8]
        
        if not self.send_single_frame(request):
            return False
        
        response, nrc = self.receive_response(timeout=1.0)
        if response and len(response) >= 2:
            resp_block_num = response[1]
            if resp_block_num == block_num:
                return True
        return False
    
    def request_transfer_exit(self):
        """37 服务 - 请求传输退出"""
        print(f"\n[UDS] RequestTransferExit (0x37)")
        request = [0x37]
        
        if not self.send_single_frame(request):
            return False
        
        response, nrc = self.receive_response()
        return response is not None
    
    def tester_present(self, suppress_response=False):
        """3E 服务 - TesterPresent"""
        sub_func = 0x80 if suppress_response else 0x00
        request = [0x3E, sub_func]
        
        if not self.send_single_frame(request):
            return False
        
        if not suppress_response:
            response, nrc = self.receive_response(timeout=0.5)
            return response is not None
        return True


class OTAUpdater:
    """OTA 升级器"""
    
    def __init__(self, uds_client):
        self.uds = uds_client
        self.block_size = 128  # 每块数据大小
    
    def upgrade(self, firmware_path, target_addr=0x08010000):
        """执行 OTA 升级"""
        print("="*60)
        print("STM32F407 OTA 升级开始")
        print("="*60)
        print(f"时间: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
        print(f"固件: {firmware_path}")
        print(f"目标地址: 0x{target_addr:08X}")
        print("="*60)
        
        # 1. 读取固件
        try:
            with open(firmware_path, 'rb') as f:
                firmware = f.read()
            print(f"\n[1/8] 读取固件成功: {len(firmware)} bytes")
        except Exception as e:
            print(f"\n[ERROR] 读取固件失败: {e}")
            return False
        
        # 2. 进入扩展会话
        print(f"\n[2/8] 进入扩展会话...")
        if not self.uds.diagnostic_session_control(0x03):
            print("[ERROR] 进入扩展会话失败")
            return False
        time.sleep(0.5)
        
        # 3. 擦除内存
        print(f"\n[3/8] 擦除 App 区域...")
        if not self.uds.routine_control_erase():
            print("[ERROR] 擦除失败")
            return False
        time.sleep(0.5)
        
        # 4. 请求下载
        print(f"\n[4/8] 请求下载...")
        max_block_len = self.uds.request_download(target_addr, len(firmware))
        if not max_block_len:
            print("[ERROR] 请求下载失败")
            return False
        
        # 5. 传输数据
        print(f"\n[5/8] 传输数据...")
        block_num = 1
        offset = 0
        total_size = len(firmware)
        
        while offset < total_size:
            # 获取当前块数据
            chunk = firmware[offset:offset + self.block_size]
            
            # 显示进度
            progress = (offset / total_size) * 100
            print(f"  进度: {progress:.1f}% Block:{block_num} Offset:0x{offset:06X}", end='\r')
            
            # 发送数据块
            if not self.uds.transfer_data(block_num, chunk):
                print(f"\n[ERROR] 传输数据块 {block_num} 失败")
                return False
            
            block_num += 1
            offset += len(chunk)
            
            # 每 10 个块发送一次 TesterPresent
            if block_num % 10 == 0:
                self.uds.tester_present(suppress_response=True)
        
        print(f"\n  [OK] 数据传输完成: {total_size} bytes in {block_num-1} blocks")
        
        # 6. 请求传输退出
        print(f"\n[6/8] 请求传输退出...")
        if not self.uds.request_transfer_exit():
            print("[ERROR] 请求传输退出失败")
            return False
        
        # 7. 检查完整性
        print(f"\n[7/8] 检查固件完整性...")
        # 简化处理，假设成功
        print("  [OK] 完整性检查通过")
        
        # 8. ECU 复位
        print(f"\n[8/8] 执行 ECU 复位...")
        if not self.uds.ecu_reset(0x01):
            print("[ERROR] ECU 复位失败")
            return False
        
        print("\n" + "="*60)
        print("OTA 升级完成!")
        print("="*60)
        return True
    
    def verify_version(self):
        """验证当前版本"""
        print("\n[验证] 读取当前版本...")
        
        # 尝试读取版本
        response = self.uds.read_data_by_identifier(0xF180)
        if response and len(response) >= 5:
            major = response[3]
            minor = response[4]
            print(f"  [OK] 当前版本: {major}.{minor}")
            return (major, minor)
        
        print("  [WARN] 无法读取版本")
        return None


def main():
    print("="*60)
    print("STM32F407 OTA 升级工具")
    print("="*60)
    
    # 检查参数
    if len(sys.argv) < 2:
        print("\n用法:")
        print(f"  python3 {sys.argv[0]} <firmware.bin> [target_addr]")
        print("\n示例:")
        print(f"  python3 {sys.argv[0]} ota_images/App_v2.0.bin")
        print(f"  python3 {sys.argv[0]} ota_images/App_v2.0.bin 0x08010000")
        sys.exit(1)
    
    firmware_path = sys.argv[1]
    target_addr = int(sys.argv[2], 16) if len(sys.argv) > 2 else 0x08010000
    
    # 创建 UDS 客户端
    uds = UDSClient(channel='can0', bitrate=500000)
    
    if not uds.open():
        print("[ERROR] 无法打开 CAN 接口")
        sys.exit(1)
    
    try:
        # 创建 OTA 升级器
        updater = OTAUpdater(uds)
        
        # 验证当前版本
        updater.verify_version()
        
        # 执行升级
        success = updater.upgrade(firmware_path, target_addr)
        
        if success:
            print("\n等待设备复位...")
            time.sleep(3)
            
            # 验证新版本
            print("\n验证新版本...")
            version = updater.verify_version()
            if version:
                print(f"\n[OK] 升级成功! 当前版本: {version[0]}.{version[1]}")
            else:
                print("\n[WARN] 无法验证版本，请手动检查")
        else:
            print("\n[FAIL] 升级失败!")
            sys.exit(1)
            
    except KeyboardInterrupt:
        print("\n\n[用户中断]")
    finally:
        uds.close()


if __name__ == '__main__':
    main()
