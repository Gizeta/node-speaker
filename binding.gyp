{
  'targets': [
    {
      'target_name': 'binding',
      'sources': [
        'src/binding.cc',
      ],
      "include_dirs" : [
        '<!(node -e "require(\'nan\')")',
        'deps/mpg123/src/compat',
        'deps/mpg123/src/libout123'
      ],
      'dependencies': [
        'deps/mpg123.gyp:module'
      ],
    }
  ]
}
