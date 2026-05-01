import socket
import matplotlib.pyplot as plt
import pandas as pd

HOST = "192.168.1.XXX"  # replace with IP of arduin
PORT = 80

data = []

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.connect((HOST, PORT))

    while True:
        line = s.recv(1024).decode()
        if not line:
            break

        lines = line.split("\n")
        for l in lines:
            if "," in l and "time" not in l:
                try:
                    t, d = l.split(",")
                    data.append([float(t), float(d)])
                except:
                    pass

df = pd.DataFrame(data, columns=["time", "depth"])
df.to_csv("float_data.csv", index=False)

plt.plot(df["time"], df["depth"])
plt.xlabel("Time (s)")
plt.ylabel("Depth (m)")
plt.title("Depth vs Time")
plt.gca().invert_yaxis()
plt.grid()

plt.show()
