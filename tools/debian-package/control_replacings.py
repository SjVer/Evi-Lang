from re import findall
from os import path

root_dir = str()
target = str()

def package_name():
	with open(path.join(root_dir, "include", "common.hpp"), "r") as f:
		return findall("#define APP_NAME \"(.+)\"", f.read())[0]

def version():
	with open(path.join(root_dir, "include", "common.hpp"), "r") as f:
		return findall("#define APP_VERSION \"(.+)\"", f.read())[0]

def architecture():
	return target

def email():
	with open(path.join(root_dir, "include", "common.hpp"), "r") as f:
		return findall("#define EMAIL \"(.+)\"", f.read())[0]

def website():
	with open(path.join(root_dir, "include", "common.hpp"), "r") as f:
		return findall("#define LINK \"(.+)\"", f.read())[0]
    

funcs = {
	"package-name": package_name,
	"version": version,
	"architecture": architecture,
	"email": email,
	"website": website,
}