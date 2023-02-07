
" Override the Build functions
compiler msvc
set makeprg=cmake

function! Clean()
  :silent make --build ..\build --target clean
endfunction

function! Build()
  :silent make --build ..\build
endfunction

function! Rebuild()
  :silent call Clean()
  :silent call Build()
endfunction

