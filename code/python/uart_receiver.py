

import serial
import numpy as np
import matplotlib.pyplot as plt

ser = serial.Serial('COM12', 1000000, timeout=5)
plt.ion()
fig, ax = plt.subplots(figsize=(5,5))

print("---> running MNIST inference...")

while True:
    line = ser.readline().decode(errors='ignore').strip()
    
    if 'DIGIT' in line:
        print(f"---> {line}")
    
    if 'FRAME_START' in line:
        data = ser.read(784)
        if len(data) == 784:
            img = np.frombuffer(data, dtype=np.uint8).reshape(28 , 28)
            ax.clear()
            ax.imshow(img, cmap='gray', vmin=0, vmax=255, interpolation='nearest')
            ax.set_title(f"prediction: {line}")
            plt.pause(0.1)

ser.close()
