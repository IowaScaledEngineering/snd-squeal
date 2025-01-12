import wave
import glob
import random
import struct
import sys

numTracks  = 4
numSeconds = 200
minSilence = 0.1
maxSilence = 10.0
gainFactor = 0.75  # Provide some attenutation to prevent clipping

wavfiles = glob.glob('16kmono/flange[0-9][0-9].wav')

outfile = "flangeMix.wav"
data1 = []
data2 = []

def readWav(logName):
    t = open(logName, "w")
    t.write(str(minSilence) + "s to " + str(maxSilence) + "s\n")
    numSamples = 0
    state = 0
    arrayOfBytes = bytearray()
    silenceDivider = 2   # Initial silence is shorter, but still random
    targetSamples = numSeconds * 16000
    while numSamples < targetSamples:
        if(0 == state):
            # Insert silence
            silenceSamples = random.randrange(int(16000*minSilence/silenceDivider), int(16000*maxSilence/silenceDivider))
            if (numSamples + silenceSamples) > targetSamples:
                break  # Don't add silence if it would put us over the target length - prevent extended silence at the end
            t.write(str(silenceSamples) + " (" + str(silenceSamples/16000) + ")\n")
            arrayOfBytes.extend(bytearray(2*silenceSamples))
            numSamples = numSamples + silenceSamples
            state = 1
            silenceDivider = 1
        else:
            # Read in wav data
            wavfile = random.choice(wavfiles)
            w = wave.open(wavfile, 'rb')
            t.write(wavfile + "\n")
            arrayOfBytes.extend(w.readframes(w.getnframes()))
            numSamples = numSamples + w.getnframes()
            w.close()
            state = 0
    print(numSamples)
    t.close()
    return arrayOfBytes

print()

byteArrays = []

for i in range(numTracks):
    byteArrays.append(readWav("flangeMix" + chr(i+ord('A')) + ".txt"))

print()

wavData = []

for b in byteArrays:
    wavData.append(list(struct.unpack('<%dh' % (len(b)//2), b)))

# Find longest one
newLength = 0
for w in wavData:
    print(len(w))
    newLength = max(newLength, len(w))
print()

# Extend all to the same (longest) length
for index, w in enumerate(wavData):
    wavData[index] = wavData[index] + [0]*(newLength - len(wavData[index]))
    print(len(wavData[index]))

output = wave.open(outfile, 'wb')
output.setnchannels(1)
output.setsampwidth(2)
output.setframerate(16000)

outputSingle = []
for index, w in enumerate(wavData):
    outputSingle.append(wave.open("flange" + chr(index+ord('A')) + ".wav", 'wb'))
    outputSingle[index].setnchannels(1)
    outputSingle[index].setsampwidth(2)
    outputSingle[index].setframerate(16000)

for i in range(newLength):
    if(0 == (i % 160000)):
        print("\n" + str(i / 16000) + "s ", end='')
        sys.stdout.flush()
    if(0 == (i % 16000)):
        print(".", end='')
        sys.stdout.flush()
    value = 0
    for j in range(len(wavData)):
        value = value + round(wavData[j][i]*gainFactor)
        outputSingle[j].writeframes(struct.pack('<h', wavData[j][i]))
    data = struct.pack('<h', value)
    output.writeframes(data)

output.close()
for index, w in enumerate(wavData):
    outputSingle[index].close()

print()
