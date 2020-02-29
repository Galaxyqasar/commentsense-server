import os
import sys
import platform
import subprocess
from shutil import *

args = sys.argv[1:]
current = os.getcwd().replace('\\', '/')
parrent = current[0:current.rfind('/')]

system = platform.system()
gcc = ""
gpp = ""
ld = ""

if system in ["Linux", "Darwin"]:
	gcc = "gcc"
	gpp = "g++"
	ld = "ld"
elif system == "Windows":
	gcc = parrent + "/gnuwin32/mingw/bin/gcc.exe"
	gpp = parrent + "/gnuwin32/mingw/bin/g++.exe"
	ld = parrent + "/gnuwin32/mingw/bin/ld.exe"

compilers = {"c" : gcc, "cpp" : gpp}
libext = {"Windows" : ".dll", "Linux" : ".so", "Darwin" : ".so"}[system]
progext = {"Windows" : ".exe", "Linux" : "", "Darwin" : ""}[system]

allmodules = ["crypt", "math", "network", "sqlite", "stb", "utils"]

check = False

def compileLib(name, rebuild, options):
	extension = name[name.rfind('.')+1:]
	compiler = compilers[extension]
	header = name[0:name.rfind('.')] + ".h" + ("pp" if (extension == "cpp") else "")
	source = name
	target = "obj/" + name[6:name.rfind('.')] + ".o"
	targetdir = target[0:target.rfind('/')]
	if not os.path.isdir(targetdir):
		os.makedirs(targetdir)
	if (not os.path.isfile(target)) or (os.path.getmtime(source) > os.path.getmtime(target)) or (os.path.isfile(header) and os.path.getmtime(header) > os.path.getmtime(target)) or rebuild:
		print("building", name, "...")
		command = [compiler, "-c", source, "-fPIC", "-o", target, ("-std=c++17" if (extension == "cpp") else "-std=gnu11"), "-Wall", "-Wno-unused-variable"] + options
		result = subprocess.run(' '.join(command), shell=True, check=check)

def linkLibs(libs, target):
	command = [ld, "-relocatable"] + libs + ["-o", target]
	subprocess.run(' '.join(command), shell=True, check=check)

def linkDll(source, target, options = []):
	soname = source[source.rfind('/'):source.rfind('.')]
	command = [gpp, source, "-shared", "-fPIC", "-o", target, "-Wl,-soname=." + soname + libext] + options
	subprocess.run(' '.join(command), shell=True, check=check)

def buildProgramm(path, yeet, options):
	command = [gpp, "./src/" + path, "-o", "build/" + path[:path.find('.')] + progext, yeet] + options
	subprocess.run(' '.join(command), shell=True, check=check)

def src(modules):
	for root, dirs, files in os.walk("./src"):
		dir = root[2:].replace('\\', '/')
		if dir[4:] in modules:
			for file in files:
				ending = file[file.find('.')+1:]
				if ending in ["cpp", "c"]:
					yield "./" + dir + '/' + file

modules = allmodules
rebuild = False
options = {"Windows" : ["-D WINDOWS"], "Linux" : ["-D LINUX"], "Darwin" : ["-D DARWIN"]}[system]
libs = []
link = {"Windows" : ["-lpthread"], "Linux" : ["-lpthread", "-ldl"]}[system]

if "network" in modules:
	if system == "Windows":
		link += {"Windows" : ["-lws2_32"], "Linux" : [], "Darwin" : []}[system]
	options += ["-D NETWORK"]

if "--rebuild" in args:
	rebuild = True

if "-s" in args or "--static-stdlib" in args:
	link += ["-static-libstdc++", "-static-libgcc"]

if "-r" in args or "--release" in args:
	options += ["-O3", "-s", "-D NO_LOG"]
elif "-d" in args or "--debug" in args:
	options += ["-g"]

if "-w" in args:
	options += ["-w"]
	check = True

if "-c" in args or "--clean" in args:
	if(os.path.isdir("obj")):
		rmtree('obj')
	if(os.path.isdir("build")):
		rmtree('build')
	exit(0)

if(os.path.isdir("build")):
	rmtree('build')
os.mkdir("build")

for file in src(modules):
	compileLib(file, rebuild, options)
	libs.append("./obj/" + file[6:file.rfind('.')] + ".o")

linkLibs(libs, "./obj/yeet.o")
linkDll("./obj/yeet.o", "./build/yeet" + libext, link)
buildProgramm("server.cpp", "./build/yeet" + libext, ["-std=gnu++17", "-lpthread", "-ldl"] + options)

compileLib("./src/plugin.cpp", rebuild, options)
compileLib("./src/server.cpp", rebuild, options)
linkDll("./obj/plugin.o", "./build/plugin" + libext)
linkDll("./obj/server.o", "./build/server" + libext)
copyfile("plugins.json", "build/plugins.json")
copyfile("init.sql", "build/init.sql")

if(os.path.isdir("www")):
	copytree("www", "build/www")
	copyfile("README.md", "build/www/README.md")
else:
	os.mkdir("build/www")

if "-e" in args:
	subprocess.run(["sudo", "./server"], cwd="./build")