import depthai as dai
pipeline  = dai.Pipeline()

script = pipeline.create(dai.node.Script)

script.setScript("""
    import time
    i = 5
    while True:
        buf = NNData(150)
        det = [i,1,0]
        i = i + 1
        buf.setLayer("fp16", det)
        node.io['out'].send(buf)
        time.sleep(1) # Sleep 1 sec
""")

spi = pipeline.createSPIOut()
spi.setStreamName("spimetaout")
spi.setBusId(0)
script.outputs['out'].link(spi.input)

xlink = pipeline.createXLinkOut()
xlink.setStreamName("test")
script.outputs['out'].link(xlink.input)

with dai.Device(pipeline) as device:
    output = device.getOutputQueue("test")
    while not device.isClosed():
        print(output.get().getData())