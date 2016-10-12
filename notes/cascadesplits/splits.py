import sys

if len(sys.argv) < 3:
    print("usage: python splits.py N lambda")

near = 2.0
far = 400.0

N = int(sys.argv[1])
L = float(sys.argv[2])
for i in range(N+1):
    logZi = near * pow(far / near, i/N)
    linZi = near + (i/N) * (far - near)
    print("Z_{},log: {}".format(i, logZi))
    print("Z_{},lin: {}".format(i, linZi))
    z = L * logZi + (1-L) * linZi
    print("Z_{}: {} ({})".format(i, z, (z - near) / (far - near)))