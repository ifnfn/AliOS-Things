src     = Split('''
        helloworld.c
''')

component = aos_component('helloworld', src)
# component.add_comp_deps('kernel/yloop', 'tools/cli')
component.add_global_macros('AOS_NO_WIFI')


dependencis = Split('''
    tools/cli 
    kernel/yloop
    framework/uService
''')
for i in dependencis:
    component.add_comp_deps(i)