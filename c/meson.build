project(
    'sserver',
    'c',
    version: '0.1',
    default_options: [
        'warning_level=3',
        'c_std=c2x',
        'b_lto=true',
    ],
)

host_system = host_machine.system()

if host_system == 'linux'
    add_global_arguments('-D_POSIX_C_SOURCE=200112L', language : 'c')
endif

thread_dep = dependency('threads')

src = files(
    'http.c',
    'log.c',
    'main.c',
    'relay.c',
    'server.c',
    'socks.c',
)

executable(
    'sserver',
    src,
    install: true,
    dependencies: [
        thread_dep,
    ],
)
