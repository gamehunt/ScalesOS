import json

with open('compile_commands.json', 'r+') as file:
    data = json.load(file)
    for entry in data:
        if entry["file"].endswith(".c"):
            entry["command"] += " -I/hdd/Projects/Scales/sysroot/include"
    file.seek(0)
    file.truncate()
    file.write(json.dumps(data, indent=4, sort_keys=True))