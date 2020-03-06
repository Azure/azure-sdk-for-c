import argparse
import os
import shutil
from subprocess import check_call

parser = argparse.ArgumentParser("Fill in a Doxyfile.template to generate docs")
parser.add_argument('--TemplateFile', default='Doxyfile.template')
parser.add_argument('--PackagePath', default='')
parser.add_argument('--PackageName', default='')
parser.add_argument('--PackageVersion', default='')
parser.add_argument('--AssetsPath', default='')
parser.add_argument('--DoxygenPath', default='c:\\program files\\doxygen\\bin\\doxygen.exe')

args = parser.parse_args()

def fill_template(template_file, package_name, package_version, package_path):
    template_contents = ''
    with open(template_file) as template:
        template_contents = template.read()

    new_content = template_contents \
        .replace('${PackageName}', package_name) \
        .replace('${Version}', package_version)

    output_path = os.path.normpath(os.path.join(package_path, 'Doxyfile'))
    with open(output_path, 'w') as output:
        output.write(new_content)

def copy_files(source_path, destination_path):
    for file_name in os.listdir(source_path):
        source_file = os.path.normpath(os.path.join(source_path, file_name))
        if os.path.isfile(source_file):
            print("copying %s to %s..." % (source_path, destination_path))
            shutil.copy(source_file, destination_path)

def invoke_doxygen(doxygen_path, working_directory, doxyfile_name='Doxyfile'):
    check_call([doxygen_path, 'Doxyfile'], cwd=working_directory)

def message(message):
    print("===============")
    print(message)
    print("===============")

package_path = os.path.join(args.PackagePath, args.PackageName)

message('Filling template')
fill_template(args.TemplateFile, args.PackageName, args.PackageVersion, package_path)

message('Copying assets')
copy_files(args.AssetsPath, package_path)

message('Invoking Doxygen')
invoke_doxygen(args.DoxygenPath, package_path)

