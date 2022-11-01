#!/bin/bash

mkdir hints

for file in $(find . -type f -name compile_commands.json); do
    uuid=$(uuidgen)
    mv ${file} hints/${uuid}.json
done

jq -s 'flatten' hints/* > compile_commands.json