import depthai as dai
import time

pipeline  = dai.Pipeline()

script = pipeline.create(dai.node.Script)

spiin_str = pipeline.create(dai.node.SPIIn)
spiin_str.setStreamName("str")
spiin_str.setBusId(0)
spiin_str.out.link(script.inputs['str'])

script.setScript("""
import time
while True:
    data = node.io['str'].get().getData()
    text = str(data, 'ascii')
    node.warn('Original string: ' + text)
    text += " World"
    node.warn('Sending back string: ' + text)

    b = Buffer(15)
    b.setData(text.encode('ascii'))
    node.io['str_out'].send(b)

    time.sleep(0.1) # Sleep 1 sec - avoid hot looping
""")

spi = pipeline.create(dai.node.SPIOut)
spi.setStreamName("ret_str")
spi.setBusId(0)
script.outputs['str_out'].link(spi.input)

with dai.Device(pipeline) as device:
    while not device.isClosed():
        time.sleep(1)