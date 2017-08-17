set nocompatible  
set magic
set completeopt=preview,menu
set backspace=2
set ignorecase smartcase   
set incsearch       
set hlsearch           
set noerrorbells    
set novisualbell           

set tags=
let g:pathToTag="/"
let g:curdir="/"

"Vundel config
filetype off
set rtp+=~/.vim/bundle/Vundle.vim
call vundle#begin()

Plugin 'VundleVim/Vundle.vim'
Plugin 'git://github.com/scrooloose/nerdtree'
Plugin 'git://github.com/majutsushi/tagbar'
Plugin 'skywind3000/asyncrun.vim'
"Plugin 'git://github.com/Valloric/YouCompleteMe'
map <C-n> :NERDTreeToggle<CR>
autocmd bufenter * if (winnr("$") == 1 && exists("b:NERDTree") && b:NERDTree.isTabTree()) | q | endif

call vundle#end()
"end config

set number
filetype plugin indent on 
set autoindent

syntax on   
colorscheme desert
set encoding=UTF-8
highlight Comment ctermfg=2 

map  <F9>  :call Comp() <CR>
map! <F9>  <Esc>:call Comp() <CR>
map  <F12> gg=G
map! <F12> <Esc>gg=G
map  <C-x> <C-v>0x
autocmd BufNewFile * exec ":call Init()"
autocmd BufReadPost * exec ":call Init()"

set shiftwidth=2            
set expandtab
set softtabstop=2           
set tabstop=2

func! Init()
  "c++ file
  if &filetype == 'cpp'
    autocmd vimenter * TagbarToggle
    map  <C-c> <C-v>0I//<Esc>j
    let g:curdir=getcwd()
    while !filereadable("./tags")
      cd ..
      if getcwd() == "/"
        break
      endif
    endwhile
    if filewritable("./tags")
      let g:pathToTag=getcwd() 
      execute ":set tags+=" . g:pathToTag . "/tags"
      autocmd BufWritePost * call UpdateCtags()
    endif
    execute ":cd " . g:curdir
  "sh file
  elseif &filetype == 'sh'
    map  <C-c> <C-v>0I#<Esc>j
  "python file
  elseif &filetype == 'python'
    autocmd vimenter * TagbarToggle
    map  <C-c> <C-v>0I#<Esc>j
  endif
endfunc

func! Comp()
	exec "w"
	if &filetype == 'cpp'
		:!g++ -std=c++11 -Wall % -o %< && ./%<
	elseif &filetype == 'sh' 
		:!bash -x %
	elseif &filetype == 'python'
		:!python %
	endif
endfunc

function! UpdateCtags()
  execute ":cd " . g:pathToTag
  if filewritable("./tags")
    :AsyncRun! ctags -R --file-scope=yes --langmap=c:+.h --languages=c,c++ --links=yes --c-kinds=+p --c++-kinds=+p --fields=+iaS --extra=+q
  endif
  execute ":cd " . g:curdir
endfunction
