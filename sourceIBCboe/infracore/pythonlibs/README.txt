##################### importing modules without installing######################
1) Can use sys to include the path containg the .py code.
2) Copy code to installation-dependent default directory.
3) Edit PYTHONPATH(an environment variable with a list of directory)

##################### Creating modules as Package #####################
A package usually corresponds to a directory with a file in it called __init__.py and any number of python files or other package directories:
structure-::
	package_name
		a_package
	   		__init__.py
   			module_a.py
   			a_sub_package
     				__init__.py
     				module_b.py
		b_package
   			__init__.py
   			module_b.py

__init__.py -> import from the modules. read by setup.py
The code will be run when the package is imported – just like a module, modules inside packages are not automatically imported. So, with the above structure: import a_package will run the code in a_package/__init__.py.

setup(name='package_name',
      version='',
      description='',
      url='',
      author='',
      author_email='',
      license='TWO ROADS PVT LTD',
      packages=['a_package','b_package','a_package.a_sub_package'],
      install_requires=[
      ],
      zip_safe=False)

import a_package; import b_package; import a_package.a_sub_package;

#INFO dependencies can be in either install requires or in Requirements.txt file

---------installing the package---------------
pip in command prompt as follows:
	pip <pip arguments>
If you cannot run the pip command directly
	python -m pip <pip arguments>

#DIR PATH
	pip install path/to/SomeProject
		    or 
	pip install -e path/to/SomeProject
-e, --editable <path/url> Install a project in editable mode (i.e. setuptools “develop mode”) from a local project path or a VCS url.
#INFO -e can be used for the debugging purpose

REQ FILE necassary if the code contain use of lib that need to be installed before your installing package (currently nill)
“Requirements files” are files containing a list of items to be installed using pip install like so:
pip install -r requirements.txt
#INFO check format here -> https://pip.pypa.io/en/stable/reference/pip_install/#requirements-file-format
