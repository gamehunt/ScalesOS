import fileinput
import re

def parse(l):
    return re.sub("\s\s+", " ", l.strip()).split(' ')

exported_symbols = []

for line in fileinput.input():
    data = parse(line)
    if len(data) != 8 or len(data[0]) == 0 or not data[0][0].isdigit():
        continue
    if data[7].endswith('exported'):
        exported_symbols.append([data[1], data[7][2:len(data[7]) - 9]])

for sym in exported_symbols:
    print('extern', sym[1])

print()
print('section .data.symbols')
print()
print('global symbols_start')
print('symbols_start:')
for sym in exported_symbols:
    print(f'dd 0x{sym[0]}')
    print(f'db \"{sym[1]}\", 0')
    print('; ---------------------')
print('global symbols_end')
print('symbols_end:')

