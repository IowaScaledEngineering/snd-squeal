import wave
import glob
import random
import struct


numSeconds = 100

wavfiles = glob.glob('flange[0-9][0-9].wav')

outfile = "flangeMix.wav"
data1 = []
data2 = []

bytearray1 = bytearray()
bytearray2 = bytearray()

t1 = open("flangeMixA.txt", "w")
t2 = open("FlangeMixB.txt", "w")

numSamples = 0
while numSamples < (numSeconds * 16000):
    wavfile = random.choice(wavfiles)
    t1.write(wavfile + "\n")
    w = wave.open(wavfile, 'rb')
    numSamples = numSamples + w.getnframes()
    data1.append( [w.getparams(), w.readframes(numSamples)] )
    w.close()

#print(numSamples)

numSamples = 0
while numSamples < (numSeconds * 16000):
    wavfile = random.choice(wavfiles)
    t2.write(wavfile + "\n")
    w = wave.open(wavfile, 'rb')
    numSamples = numSamples + w.getnframes()
    data2.append( [w.getparams(), w.readframes(numSamples)] )
    w.close()

#print(numSamples)

for i in range(len(data1)):
    bytearray1.extend(data1[i][1])

for i in range(len(data2)):
    bytearray2.extend(data2[i][1])

#print(len(bytearray1))
#print(len(bytearray2))

wavData1 = list(struct.unpack('<%dh' % (len(bytearray1)//2), bytearray1))
wavData2 = list(struct.unpack('<%dh' % (len(bytearray2)//2), bytearray2))

#print(len(wavData1))
#print(len(wavData2))
#print(wavData1)
#print(wavData2)

if len(wavData1) > len(wavData2):
    wavData2 = wavData2 + [0]*(len(wavData1) - len(wavData2))
elif len(wavData2) > len(wavData1):
    wavData1 = wavData1 + [0]*(len(wavData2) - len(wavData1))

print(len(wavData1))
print(len(wavData2))
#print(wavData1)
#print(wavData2)

output = wave.open(outfile, 'wb')
output.setnchannels(1)
output.setsampwidth(2)
output.setframerate(16000)
for i in range(len(wavData1)):
    value = (wavData1[i] + wavData2[i])//2
    data = struct.pack('<h', value)
    output.writeframes(data)
output.close()
    
t1.close()
t2.close()
