CuraEngine_onylfans

NOTE: Please note that this plugin is Experimental and adding custom infills is not possible at the moment. This plugin consist of the following licenses:

    LGPLv3 front end Cura plugin.
    BSD-4 C++ Business logic headers for the CuraEngine plugin logic
    AGPLv3 C++ Infill generation header

Installation

    Configure Conan Before you start, if you use conan for other (big) projects as well, it's a good idea to either switch conan-home and/or backup your existing conan configuration(s).

That said, installing our config goes as follows:

pip install conan==1.60
conan config install https://github.com/ultimaker/conan-config.git
conan profile new default --detect --force

    Clone CuraEngine_plugin_infill_generate

https://github.com/Ultimaker/CuraEngine_plugin_infill_generate.git
cd CuraEngine_plugin_infill_generate

    Install & Build CuraEngine (Release OR Debug)

conan install . --build=missing --update -s build_type=Debug/Release