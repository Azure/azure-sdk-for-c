# Fills in Doxyfile.template with

import argparse

parser = argparse.ArgumentParser("Fill in a Doxyfile.template to generate docs")
parser.add_argument('--InputFile', default='Doxyfile.template')
parser.add_argument('--OutputFile', default='Doxyfile')
parser.add_argument('--PackageName', default='')
parser.add_argument('--PackageVersion', default='')

args = parser.parse_args()

template_contents = ''
with open(args.InputFile) as template:
    template_contents = template.read()

new_content = template_contents \
    .replace('${PackageName}', args.PackageName) \
    .replace('${Version}', args.PackageVersion)

with open(args.OutputFile, 'w') as output:
    output.write(new_content)
