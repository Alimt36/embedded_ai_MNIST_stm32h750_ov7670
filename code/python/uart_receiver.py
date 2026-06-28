

# import serial
# import numpy as np
# import matplotlib.pyplot as plt

# ser = serial.Serial('COM12', 1000000, timeout=5)
# plt.ion()
# fig, ax = plt.subplots(figsize=(5,5))

# print("---> running MNIST inference...")

# while True:
#     line = ser.readline().decode(errors='ignore').strip()
    
#     if 'DIGIT' in line:
#         print(f"---> {line}")
    
#     if 'FRAME_START' in line:
#         data = ser.read(784)
#         if len(data) == 784:
#             img = np.frombuffer(data, dtype=np.uint8).reshape(28 , 28)
#             ax.clear()
#             ax.imshow(img, cmap='gray', vmin=0, vmax=255, interpolation='nearest')
#             ax.set_title(f"prediction: {line}")
#             plt.pause(0.1)

# ser.close()



import serial
import numpy as np
import matplotlib.pyplot as plt

#---------------------------------------------------------------------------------------------------------------------------
PORT     = 'COM12'
BAUDRATE = 1000000
#---------------------------------------------------------------------------------------------------------------------------

#---------------------------------------------------------------------------------------------------------------------------
def __main__():
    ser = serial.Serial(PORT, BAUDRATE, timeout=10)
    print("---> streaming...")

    plt.ion()
    fig, axes = plt.subplots(1, 2, figsize=(12, 5))
    fig.suptitle("STM32H750 MNIST Live Inference", fontsize=14)

    predicted = "?"

    while True:
        line = ser.readline().decode(errors='ignore').strip()

        if 'DIGIT' in line:
            predicted = line.split(':')[1]
            print(f"---> {line}")

        if 'FRAME_START' in line:
            data = ser.read(784)
            if len(data) == 784:
                img = np.frombuffer(data, dtype=np.uint8).reshape(28, 28)
                axes[0].clear()
                axes[0].imshow(img, cmap='gray', vmin=0, vmax=255, interpolation='nearest')
                axes[0].set_title(f"28x28 input  |  prediction: {predicted}")
                axes[0].axis('off')

        if 'RAW_START' in line:
            data = ser.read(160 * 120 * 2)
            if len(data) == 160 * 120 * 2:
                Y = np.frombuffer(data, dtype=np.uint8)[1::2].reshape(120, 160)
                axes[1].clear()
                axes[1].imshow(Y, cmap='gray', vmin=0, vmax=255)
                axes[1].set_title("raw camera feed")
                axes[1].axis('off')
                plt.tight_layout()
                plt.pause(0.05)

    ser.close()

__main__()
#---------------------------------------------------------------------------------------------------------------------------