import numpy as np
import matplotlib.pyplot as plt

# alpha = n1/n2 = 1/n2 in air
# assuming n2 is always denser than air (n2 > n1 = 1)

def schlick(theta, n2):
    R0 = (n2-1)/(n2+1)
    R0 = R0*R0
    return R0 + (1-R0) * (1 - np.cos(theta))**5

def fresnel(theta, n2):
    A = np.cos(theta) / n2
    B = np.sqrt(1 - (np.sin(theta) / n2)**2)
    R = (A - B) / (A + B)
    return R*R

def schlick_f0(theta, f0):
    R0 = f0
    exponent = 3 - f0 * 2 # f0 = 0 -> 3, f0 = 1 -> 1
    return R0 + (1-R0) * (1 - np.cos(theta))**exponent

def fresnel_f0(theta, f0):
    n2 = (1 + np.sqrt(f0)) / (1 - np.sqrt(f0))
    return fresnel(theta, n2)

col_schlick = "blue"
col_fresnel = "red"
thetas = np.linspace(0, np.pi/2, 200)

# for n2 in np.linspace(1.0, 2.0, 5):
#     plt.text(0, schlick(0, n2), "n2: " + str(n2))
#     plt.plot(thetas, schlick(thetas, n2), color = col_schlick)
#     plt.plot(thetas, fresnel(thetas, n2), color = col_fresnel)
# plt.show()

for f0 in [0.0, 0.02, 0.04, 0.05, 0.07, 0.08, 0.5, 0.8, 1.0][:-2]:
    plt.text(0, f0, "f0 = " + str(f0))
    plt.plot(thetas, schlick_f0(thetas, f0), color = col_schlick)
    plt.plot(thetas, fresnel_f0(thetas, f0), color = col_fresnel)
plt.show()

# Result:
# Schlick only seems to be *somewhat* accurate if n_1/n_2 < 1 and then the best fit seems to be around 0.8
# This would correspond to a refraction from air to n_2 = ~1.25

# This seems very good as well: https://seblagarde.wordpress.com/2013/04/29/memo-on-fresnel-equations/
