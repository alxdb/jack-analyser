env = Environment()
pkg_config_libs = ['gtkmm-3.0', 'glew', 'jack']
env.ParseConfig("pkg-config --cflags --libs %s" % ' '.join(pkg_config_libs))
env.MergeFlags('-g')
env.Program('main', ['main.cpp', 'scope.cpp'])
