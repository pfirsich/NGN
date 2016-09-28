import numpy as np
import matplotlib.pyplot as plt

def schlick(theta, alpha):
    R0 = (1-alpha)/(1+alpha)
    R0 = R0*R0
    return R0 + (1-R0) * (1 - np.cos(theta))**5
    
def fresnel(theta, alpha):
    A = alpha * np.cos(theta)
    B = np.sqrt(1 - (alpha * np.sin(theta))**2)
    R = (A - B) / (A + B)
    return R*R
 
col_schlick = "blue"
col_fresnel = "red"
thetas = np.linspace(0, np.pi/2, 200) 
for alpha in [0.8]: #np.linspace(0.6, 1.0, 3):
    plt.text(0, schlick(0, alpha), str(alpha))
    plt.plot(thetas, schlick(thetas, alpha), color = col_schlick)
    plt.plot(thetas, fresnel(thetas, alpha), color = col_fresnel)
    
# Result:
# Schlick only seems to be *somewhat* accurate if n_1/n_2 < 1 and then the best fit seems to be around 0.8
# This would correspond to a refraction from air to n_2 = ~1.25

# This seems very good as well: https://seblagarde.wordpress.com/2013/04/29/memo-on-fresnel-equations/
    
plt.show()