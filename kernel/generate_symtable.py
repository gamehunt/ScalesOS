import fileinput
import re

exported_symbols = []
blacklisted_symbols = ["symbols_start", "symbols_end"]

def parse(l):
    return re.sub("\s\s+", " ", l.strip()).split(' ')

for line in fileinput.input():
    data = parse(line)
    if len(data) != 8 or len(data[0]) == 0 or not data[0][0].isdigit() or data[7] in blacklisted_symbols or data[6] == "ABS":
        continue
    exported_symbols.append([data[1], data[7]])

print()
print('section .data.symbols')
print()
print('global symbols_start')
print('symbols_start:')
for sym in exported_symbols:
    print(f'dd 0x{sym[0]}')
    print(f'db \"kernel\"')
    print(f'times 26 db 0')
    print(f'db \"{sym[1]}\", 0')
    print('; ---------------------')
print('global symbols_end')
print('symbols_end:')

