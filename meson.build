project('TP3_4', 'c')

ld = [
    '-lrt',
    '-lpthread'
]

src_gvf = [
    'src/gen_vec_file.c',
    'lib/posix_helper.h',
    'lib/posix_helper.c'
]

lib = include_directories('lib')
 
executable('posix1', 'src/posix1.c', link_args : ld)
executable('posix2', 'src/posix2.c', link_args : ld)
executable('posix3', 'src/posix3.c', link_args : ld)
executable('posix4', 'src/posix4.c', link_args : ld)
executable('posix5', 'src/posix5.c', link_args : ld)

executable('gen_vec_file', src_gvf, include_directories: lib)