src =Split(''' 
    service.c
''')
component =aos_component('uService', src)


global_includes =Split(''' 
    .
''')
for i in global_includes:
    component.add_global_includes(i)

global_macros =Split(''' 
    AOS_USERVICE
''')
for i in global_macros:
    component.add_global_macros(i)

for i in includes:
    component.add_includes(i)


