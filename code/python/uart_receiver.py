# import serial
# import numpy as np
# import matplotlib.pyplot as plt

# ser = serial.Serial('COM12', 115200, timeout=5)
# print("---> waiting for data...")

# # ---> skip any text messages first
# while True:
#     line = ser.readline()
#     if b'CAM_OK' in line:
#         print("---> camera OK, waiting for image...")
#         break

# data = ser.read(196)
# print(f"---> got {len(data)} bytes")

# if len(data) == 196:
#     img = np.frombuffer(data, dtype=np.uint8).reshape(14, 14)
#     plt.figure(figsize=(4,4))
#     plt.imshow(img, cmap='gray', vmin=0, vmax=255)
#     plt.title("14x14 camera output")
#     plt.colorbar()
#     plt.show()

# ser.close()













# import serial
# import numpy as np
# import matplotlib.pyplot as plt

# ser = serial.Serial('COM12', 1000000, timeout=5)
# plt.ion()
# fig, ax = plt.subplots(figsize=(5,5))

# print("---> streaming 14x14...")

# while True:
#     line = ser.readline()
#     if b'FRAME_START' in line:
#         data = ser.read(196)
#         if len(data) == 196:
#             img = np.frombuffer(data, dtype=np.uint8).reshape(14, 14)
#             ax.clear()
#             ax.imshow(img, cmap='gray', vmin=0, vmax=255, interpolation='nearest')
#             ax.set_title("14x14 live")
#             plt.pause(0.1)

# ser.close()






























# import serial
# import numpy as np
# import matplotlib.pyplot as plt

# # ser = serial.Serial('COM12', 115200, timeout=10)
# # ser = serial.Serial('COM12', 921600, timeout=10)
# ser = serial.Serial('COM12', 1000000, timeout=10)
# plt.ion()
# fig, ax = plt.subplots(figsize=(6, 5))

# print("---> streaming... adjust focus now!")

# while True:
#     line = ser.readline()
#     if b'FRAME_START' in line:
#         data = ser.read(160 * 120 * 2)
#         if len(data) == 160 * 120 * 2:
#             Y = np.frombuffer(data, dtype=np.uint8)[1::2].reshape(120, 160)
#             ax.clear()
#             ax.imshow(Y, cmap='gray')
#             ax.set_title("live feed - adjust focus!")
#             plt.pause(0.1)

# ser.close()












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
        data = ser.read(196)
        if len(data) == 196:
            img = np.frombuffer(data, dtype=np.uint8).reshape(14, 14)
            ax.clear()
            ax.imshow(img, cmap='gray', vmin=0, vmax=255, interpolation='nearest')
            ax.set_title(f"prediction: {line}")
            plt.pause(0.1)

ser.close()