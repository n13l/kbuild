filetype on                                                                     
filetype plugin on                                                              
filetype indent on                                                              
syntax on                                                                       
set fileencodings=utf-8

set number                                                                      
set statusline=%<%f%=%([%{Tlist_Get_Tagname_By_Line()}]%)
set nocompatible
set statusline+=%o
set ruler                                                                       
set showmode                                                                    
set backspace=2                                                                 
set nobackup                                                                    
set nowb                                                                        
set noswapfile
set ignorecase

set t_Co=256

if &term=~'screen-256color' 
	colo ron
	hi LineNr term=bold ctermfg=grey guifg=grey gui=bold guifg=grey
endif

if &term=~'xterm' 
	colo ron
	hi LineNr term=bold ctermfg=grey guifg=grey gui=bold guifg=grey
endif

if &term=~'xterm-256color' 
	colo ron
	hi LineNr term=bold ctermfg=grey guifg=grey gui=bold guifg=grey
endif


if has("unix")                                                                  
	let s:uname = system("uname")                                           
	if s:uname == "Linux\n"
"		colo ron
	endif                                                                   
endif

"hi CursorLine     guibg=green gui=none
"hi LineNr         ctermfg=green guifg=green guibg=green
"hi CursorLineNr   term=bold ctermfg=green gui=bold guifg=green
hi LineNr term=bold ctermfg=grey guifg=grey gui=bold guifg=grey
"hi Statement term=bold ctermfg=blue guifg=#80a0ff gui=bold

noremap <F8> :set list!<CR>
map <C-c> "+y<CR>

hi NonText guifg=#4a4a59
hi SpecialKey guifg=#4a4a59
hi Search ctermfg=white ctermbg=black cterm=bold

set listchars=eol:$,tab:>-,trail:~,extends:>,precedes:<
set backspace=indent,eol,start
set cc=80

hi OverLength ctermbg=black ctermfg=white guibg=black
match OverLength /\%>80v.\+/
"set ignorecase "Ignore case when searching
set smartcase
set hlsearch "Highlight search things
"set viminfo='50,\"500 set history=50

hi PMenu ctermbg=black ctermfg=white
hi PMenuSel ctermbg=white ctermfg=black

let g:snips_author = 'Daniel Kubec'
let g:snips_email = 'niel@rtfm.cz'
let g:snips_copyright = 'Example Corp.'

" Complete options (disable preview scratch window)
set completeopt=menu,menuone,longest
" Limit popup menu height
"set pumheight = 15
" SuperTab option for context aware completion
let g:SuperTabDefaultCompletionType = "context"
" Disable auto popup, use <Tab> to autocomplete
let g:clang_complete_auto = 0
" Show clang errors in the quickfix window
let g:clang_complete_copen = 1
let g:clang_snippets = 1
let g:clang_snippets_engine = 'clang_complete'

if has("unix")
	let g:clang_use_library = 1
	let s:uname = system("uname")
	if s:uname == "Darwin\n"
"		colo default
		let g:clang_use_library = 1
		let g:clang_library_path = '/Library/Developer/CommandLineTools/usr/lib/libclang.dylib'
	endif
endif

if has('win32unix') " Cygwin
	let g:clang_use_library = 0
	let g:clang_library_path = '/usr/lib/libclang.dll'
	let g:clang_auto_user_options='path, .clang_complete'
endif


"let g:clang_conceal_snippets = 1
let g:clang_debug=0
let g:clang_hl_errors=0
let g:clang_user_options='|| exit 0'
let g:clang_complete_auto = 0
let g:clang_complete_copen = 0
let g:clang_complete_macros = 1
set conceallevel=2
set concealcursor=vin
"let g:SuperTabDefaultCompletionType='<c-x><c-u><c-p>'
let g:CCTreeKeyTraceForwardTree = '<C-\>>' 
let g:CCTreeKeyTraceReverseTree = '<C-\><' 
let g:CCTreeKeyHilightTree = '<C-l>'        " Static highlighting
let g:CCTreeKeySaveWindow = '<C-\>y' 
let g:CCTreeKeyToggleWindow = '<C-\>w' 
let g:CCTreeKeyCompressTree = 'zs'     " Compress call-tree 
let g:CCTreeKeyDepthPlus = '<C-\>=' 
let g:CCTreeKeyDepthMinus = '<C-\>-'

set foldcolumn=0
hi FoldColumn ctermbg=NONE ctermfg=NONE
"hi StatusLine ctermbg=White ctermfg=Black
hi ColorColumn ctermbg=DarkGray  guibg=DarkGray
"hi CursorLine                guibg=#1c1c1c gui=NONE ctermbg=234 cterm=NONE
"hi CursorLineNr              guifg=#a9a8a8 gui=NONE ctermfg=248 cterm=NONE
"hi ColorColumn               guibg=#1c1c1c ctermbg=234
"hi! link CursorColumn ColorColumn
"hi VertSplit                 guifg=#444444 guibg=#121212 gui=NONE ctermfg=238 ctermbg=233 cterm=NONE
"hi SignColumn                guifg=#FFFFFF guibg=NONE ctermfg=15 ctermbg=NONE
hi StatusLine                guifg=#e4e4e4 guibg=#606060 gui=NONE ctermfg=254 ctermbg=241 cterm=NONE
hi StatusLineNC              guifg=#585858 guibg=#303030 gui=NONE ctermfg=240 ctermbg=236 cterm=NONE
" line used for closed folds
hi Folded                    guifg=#ffffff guibg=#444444 gui=NONE ctermfg=15 ctermbg=238 cterm=NONE
hi! link FoldColumn SignColumn
"hi NonText                   guifg=#767676 gui=NONE cterm=NONE ctermfg=243
"hi SpecialKey                guifg=#767676 gui=NONE cterm=NONE ctermfg=243

let winManagerWindowLayout = 'FileExplorer|TagList'
let Tlist_WinWidth=40
let Tlist_Enable_Fold_Column=0

"let Tlist_Use_Right_Window   = 1
nnoremap <F2> :TlistOpen<CR>
nnoremap <F3> :TlistToggle<CR>
nnoremap <F4> :NERDTreeToggle<CR> 
nnoremap <F5> :CCTreeLoadDB<CR>
nnoremap <F6> :call g:ClangUpdateQuickFix()<CR>
"autocmd vimenter * NERDTree
"autocmd StdinReadPre * let s:std_in=1
"autocmd VimEnter * if argc() == 0 && !exists("s:std_in") | NERDTree | endif
"
let NERDTreeMinimalUI=1
"autocmd VimEnter * execute 'NERDTree' | wincmd p
"autocmd bufenter * if (winnr("$") == 1 && exists("b:NERDTreeType") && b:NERDTreeType == "primary") | q | endif
"let NERDTreeIgnore=['\.\.$', '\.$', '\~$']
"
