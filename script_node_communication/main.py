import depthai as dai
import time

pipeline  = dai.Pipeline()

script = pipeline.create(dai.node.Script)

# spiin_num = pipeline.create(dai.node.SPIIn)
# spiin_num.setStreamName("num")
# spiin_num.setBusId(0)
# spiin_num.out.link(script.inputs['num'])

spiin_str = pipeline.create(dai.node.SPIIn)
spiin_str.setStreamName("str")
spiin_str.setBusId(0)
spiin_str.out.link(script.inputs['str'])

script.setScript("""
import time
while True:
    data = node.io['str'].get().getData()
    str = str(data, 'ascii')
    node.warn('Original string: ' + str)
    str += " World"
    node.warn('Sending back string: ' + str)

    b = Buffer(15)
    b.setData(str.encode('ascii'))
    node.io['str_out'].send(b)

    time.sleep(0.1) # Sleep 1 sec - avoid hot looping
""")

spi = pipeline.create(dai.node.SPIOut)
spi.setStreamName("ret_str")
spi.setBusId(0)
script.outputs['str_out'].link(spi.input)

# xlink = pipeline.create(dai.node.XLinkOut)
# xlink.setStreamName("test")
# script.outputs['out'].link(xlink.input)

with dai.Device(pipeline) as device:
    while not device.isClosed():
        time.sleep(1)