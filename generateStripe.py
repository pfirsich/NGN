import sys
from PIL import Image

imagePath = sys.argv[1]
margin = None
if len(sys.argv) > 2:
    margin = sys.argv[2]

outSize = (900, 250)
aspect = outSize[1] / outSize[0]

image = Image.open(imagePath)
height = int(image.size[0]*aspect)
y = (image.size[1] - height) / 2
if margin:
    if margin.endswith("%"):
        margin = max(-100, min(100, int(margin[:-1]))) / 100.0
        print("margin: ", margin)
        y += margin * y
    else:
        y += int(margin)

image = image.crop((0, y, image.size[0], y + height)).resize(outSize, resample=Image.LANCZOS)
image.save("stripe.png")