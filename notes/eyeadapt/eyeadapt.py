import numpy as np
import matplotlib.pyplot as plt

dts = np.linspace(0.1, 1.0, 6)
for dt in dts:
    v = [0]
    vp = [0]
    t = [0]
    while t[-1] < 20.0:
        t.append(t[-1] + dt)
        v.append(v[-1] + (1.0 - v[-1]) * 0.1 * dt)
        vp.append(vp[-1] + (1.0 - vp[-1]) * (1.0 - np.exp(-dt*0.1)))
    #plt.plot(t, v)
    plt.plot(t, vp)
plt.show()