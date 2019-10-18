project('TP3_4', 'c')

ld = [
    '-lrt',
    '-lpthread'
]

src_pos_help = [
    'lib/posix_helper.h',
    'lib/posix_helper.c'
]

lib = include_directories('lib')
 
executable('posix1', 'src/posix1.c', link_args : ld, include_directories: lib)
executable('posix2', 'src/posix2.c', link_args : ld, include_directories: lib)
executable('posix3', 'src/posix3.c', link_args : ld, include_directories: lib)
executable('posix4', ['src/posix4.c', src_pos_help], link_args : ld, include_directories: lib)
executable('posix5', ['src/posix5.c', src_pos_help], link_args : ld, include_directories: lib)

executable('gen_vec_file', ['src/gen_vec_file.c', src_pos_help], include_directories: lib)