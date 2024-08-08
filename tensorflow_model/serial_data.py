import serial
import pandas as pd
import time

# 配置串口参数
port = 'COM5'  # 替换为你的ESP32连接的串口号
baud_rate = 115200  # 替换为你的ESP32的波特率

data_x = 'data_x.csv'
data_y = 'data_y.csv'
label = [8]

try:
    # 打开串口
    ser = serial.Serial(port, baud_rate)
    print(f"串口 {port} 打开成功，波特率 {baud_rate}")

    # 打开或创建CSV文件
    with open(data_x, mode='a', newline='') as file1, open(data_y, mode='a', newline='') as file2:
        while True:
            input('输入任意键开始一次采集')
            ser.write("1".encode());  # 发送数据请求指令
            time.sleep(0.1) # 等待数据

            # 读取串口数据
            if ser.in_waiting > 0:
                line = ser.readline().decode('utf-8').strip()
                if line.endswith(','):
                    line = line[:-1]
                print(f"读取到数据: {line}")  # 打印读取到的数据

                try:
                    # 将数据按逗号分割并转换为float32
                    data = [float(x) for x in line.split(',')]

                    # 将数据转换为DataFrame
                    df_x = pd.DataFrame([data])
                    df_y = pd.DataFrame([label])

                    # 追加数据到CSV文件
                    df_x.to_csv(file1, header=False, index=False)
                    df_y.to_csv(file2, header=False, index=False)

                    # 刷新文件缓冲区
                    file1.flush()
                    file2.flush()
                    print("数据追加到CSV文件成功")

                except ValueError as e:
                    print(f"数据转换错误: {e}")

except serial.SerialException as e:
    print(f"串口打开失败: {e}")
except Exception as e:
    print(f"发生错误: {e}")
