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

# ser = serial.Serial('COM12', 115200, timeout=10)
# print("waiting for frame...")

# while True:
#     line = ser.readline()
#     if b'FRAME_START' in line:
#         break

# data = ser.read(160 * 120 * 2)
# print(f"got {len(data)} bytes")

# # ---> show raw Y channel even bytes
# Y_even = np.frombuffer(data, dtype=np.uint8)[0::2].reshape(120, 160)
# # ---> show raw Y channel odd bytes
# Y_odd  = np.frombuffer(data, dtype=np.uint8)[1::2].reshape(120, 160)

# fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(12, 5))
# ax1.imshow(Y_even, cmap='gray')
# ax1.set_title("even bytes")
# ax2.imshow(Y_odd, cmap='gray')
# ax2.set_title("odd bytes")
# plt.show()

# ser.close()






























import serial
import numpy as np
import matplotlib.pyplot as plt

ser = serial.Serial('COM12', 115200, timeout=10)
plt.ion()
fig, ax = plt.subplots(figsize=(6, 5))

print("---> streaming... adjust focus now!")

while True:
    line = ser.readline()
    if b'FRAME_START' in line:
        data = ser.read(160 * 120 * 2)
        if len(data) == 160 * 120 * 2:
            Y = np.frombuffer(data, dtype=np.uint8)[1::2].reshape(120, 160)
            ax.clear()
            ax.imshow(Y, cmap='gray')
            ax.set_title("live feed - adjust focus!")
            plt.pause(0.1)

ser.close()