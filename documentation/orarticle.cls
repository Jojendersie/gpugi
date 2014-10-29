\NeedsTeXFormat{LaTeX2e}
\ProvidesClass{orarticle}[12/07/2014, v1.1]

\newif\ifprint
\DeclareOption{print}{%
  \def\print{...}%
  \printtrue
}
\ExecuteOptions*
\ProcessOptions\relax

\ifprint
    \PassOptionsToClass{twoside}{article}
\fi
\DeclareOption*{\PassOptionsToClass{\CurrentOption}{article}}
\ExecuteOptions*
\ProcessOptions\relax
\LoadClass{article}

% Defining print option
\ifprint
	\RequirePackage[top=2.0cm,bottom=2.5cm,inner=3.5cm,outer=2.1cm]{geometry}
	\newcommand{\varclearpage}{\cleardoublepage}
	\RequirePackage{emptypage}	% Don't put numbers on empty pages
	\PassOptionsToClass{colorscheme=grey}{orarticle}
\else
	\RequirePackage[top=2.0cm,right=2.0cm,bottom=2.5cm,left=2.0cm]{geometry}
	\newcommand{\varclearpage}{\clearpage}
\fi

% Defining the color scheme option
\RequirePackage{xkeyval}
\RequirePackage{color}
\define@choicekey*{oropt}{colorscheme}[\val\nr]{green,darkblue,grey}[green]{
    \ifcase\nr\relax
	    \definecolor{col_cite}{rgb}{ 0.1, 0.4, 0.1 }
        \definecolor{col_sectionbox}{rgb}{ 0.6, 0.9, 0.6 }
		\definecolor{col_section}{rgb}{ 0.0, 0.0, 0.0 }
        \definecolor{col_subsectionbox}{rgb}{ 0.7, 0.9, 0.7 }
		\definecolor{col_subsection}{rgb}{ 0.0, 0.0, 0.0 }
        \definecolor{col_lightback}{rgb}{ 0.9, 0.96, 0.9 }
	\or
	    \definecolor{col_cite}{rgb}{ 0.1, 0.1, 0.4 }
        \definecolor{col_sectionbox}{rgb}{ 0.14, 0.19, 0.3 }
        \definecolor{col_section}{rgb}{ 1.0, 1.0, 0.9 }
        \definecolor{col_subsectionbox}{rgb}{ 0.3333, 0.384, 0.44 }
		\definecolor{col_subsection}{rgb}{ 1.0, 1.0, 1.0 }
        \definecolor{col_lightback}{rgb}{ 0.9, 0.9, 0.96 }
	\or
	    \definecolor{col_cite}{gray}{ 0.2 }
        \definecolor{col_sectionbox}{gray}{ 0.8 }
        \definecolor{col_section}{gray}{ 0.0 }
        \definecolor{col_subsectionbox}{gray}{ 0.9 }
		\definecolor{col_subsection}{gray}{ 0.0 }
        \definecolor{col_lightback}{gray}{ 0.95 }
    \fi
}
\ExecuteOptionsX<oropt>{colorscheme=green}
\ProcessOptionsX<oropt>

% Defining columness option
\@ifclasswith{article}{twocolumn}
{%
	\newcommand{\varcolumn}{\twocolumn}
}{%
	\newcommand{\varcolumn}{\onecolumn}
}

% Remove warning of unused options from article
\DeclareOption*{}
\ProcessOptions\relax

% Import coding and symbols
\RequirePackage{babel}
\RequirePackage[utf8]{inputenc}
\RequirePackage{graphicx}
\RequirePackage{enumitem}
\RequirePackage{amsmath}
\RequirePackage{setspace}

% Equations
\numberwithin{equation}{section}

% format captions: small, compact blocks
\RequirePackage[font=small,labelfont=bf,format=plain,margin=5pt,labelsep=space]{caption}
\RequirePackage{needspace}

% colors for the docuument
\definecolor{darkgrey}{rgb}{ 0.1, 0.1, 0.1 }

% format the table of contents
\RequirePackage{titletoc}
\titlecontents{chapter}[0.5em]{\addvspace{1pc}\bfseries}{\contentslabel{0.5em}}{}{\titlerule*[0.3pc]{.}\contentspage}
\titlecontents{section}[1.5em]{\addvspace{4pt}}{\bf\contentslabel{1.0em}}{}{\titlerule*[0.3pc]{.}\contentspage}
\titlecontents{subsection}[3.0em]{}{\contentslabel{1.5em}}{}{\titlerule*[0.3pc]{.}\contentspage}
\titlecontents{subsubsection}[5.1em]{}{\contentslabel{2.3em}}{}{\titlerule*[0.3pc]{.}\contentspage}
\titlecontents{paragraph}[7.8em]{}{\contentslabel{2.7em}}{}{\titlerule*[0.3pc]{.}\contentspage}

% include hyperref as last one (recommended)
\RequirePackage[colorlinks=true,pdfborder={0 0 0},
		linkcolor=darkgrey,
		citecolor=col_cite,
		pdfstartview=FitH]{hyperref}

% Paragraph Style
\parindent 0cm                        % No indentation of paragraphes
%\parskip0.75ex plus0.5ex minus0.25ex  % Vertical space between paragraphes
\parskip0.85ex                        % Vertical space between paragraphes

		
		
\makeatletter
	% Section commands, which add a colored background
	
	\newcommand{\sectionStar}[1]{
		%\par\noindent
		%\pagebreak[3]
		\needspace{4ex}
		\hspace*{-3pt}
		\colorbox{col_sectionbox}{\parbox[t][2.3ex][c]{\dimexpr\linewidth-6pt}{ \sffamily\normalsize\bfseries\color{col_section} #1 }}
		\par\nopagebreak
	}
	\newcommand{\sectionNoStar}[1]{
		\vspace{0.3cm}
		\refstepcounter{section}
		\sectionStar{\thesection \hspace{0.2cm} #1}
		\addcontentsline{toc}{section}{\protect\numberline{\thesection} #1}
	}
	\renewcommand{\section}{\@ifstar \sectionStar \sectionNoStar}

	\newcommand{\subsectionStar}[1]{
		%\par\noindent
		\needspace{4ex}
		\hspace*{-3pt}
		\colorbox{col_subsectionbox}{\parbox[t][2ex][c]{\dimexpr\linewidth-6pt}{ \sffamily\normalsize\color{col_subsection} #1 }}
		\par\nopagebreak
	}
	\newcommand{\subsectionNoStar}[1]{
		\vspace{0.1cm}
		\refstepcounter{subsection}
		\subsectionStar{\thesubsection \hspace{0.2cm} #1}
		\addcontentsline{toc}{subsection}{\protect\numberline{\thesubsection} #1}
	}
	\renewcommand{\subsection}{\@ifstar \subsectionStar \subsectionNoStar}
	
	\newcommand{\subsubsectionStar}[1]{
		%\par\noindent
		\needspace{4ex}
		\hspace*{-3pt}
		\colorbox{col_lightback}{\parbox[t][1.8ex][c]{\dimexpr\linewidth-6pt}{ \sffamily\small #1 }}
		\par\nopagebreak
	}
	\newcommand{\subsubsectionNoStar}[1]{
		\refstepcounter{subsubsection}
		\subsubsectionStar{\thesubsubsection \hspace{0.2cm} #1}
		\addcontentsline{toc}{subsubsection}{\protect\numberline{\thesubsubsection} #1}
	}
	\renewcommand{\subsubsection}{\@ifstar \subsubsectionStar \subsubsectionNoStar}

	\newcommand{\nicehline}[1][1ex]{
		\begin{center}
		\textcolor{col_subsectionbox}{\rule{0.45\linewidth}{\dimexpr #1/2}}
		\textcolor{col_subsectionbox}{\rule[\dimexpr -#1/4]{#1}{#1}}
		\textcolor{col_subsectionbox}{\rule{0.45\linewidth}{\dimexpr #1/2}}
		\end{center}
	}
	
	\newcommand{\authors}[1]{\def\@authors{#1}}
	\newcommand{\supervisors}[1]{\def\@supervisors{#1}}
	\newcommand{\titlehead}[1]{\def\@titlehead{#1}}
	\newcommand{\subject}[1]{\def\@subject{#1}}
	\renewcommand{\abstract}[2]{\def\@abstrtitel{#1} \def\@abstract{#2}}
	\newcommand{\keywords}[1]{\def\@keywords{#1}}
	\renewcommand{\maketitle}{
		\varcolumn[{
			\thispagestyle{empty}
			\begin{center}
				\ifx\@subject\@undefined  \else
			 	    \textit{\small \@subject}
					\vspace{0.3cm}
				\fi
				
				{\huge \textbf{ \@title} \par}
				
				\vspace{0.3cm}
				\@authors\\
				\today
			\end{center}
	
			
			\section*{\@abstrtitel}
			%\vspace*{-0.3cm}
			\colorbox{col_lightback}{\parbox{\dimexpr\linewidth-6pt}{
				\@abstract
			}}
			\vspace{0.7cm}
		}]
		
		%\varcolumn{
		%	\topskip0pt
		%	\vspace*{\fill}
		%	\ifx\@titlehead\@undefined  \else
		%	    \textit{\normalsize \@titlehead}
		%		\vspace{1.0cm}
		%	\fi
			
		%	\nicehline[1.5ex]
		%	\vspace{1.5cm}
		%	
		%	\begin{center}
		%		\ifx\@subject\@undefined  \else
		%	 	    \textit{\Large \@subject}
		%			\vspace{1.5cm}
		%		\fi
			 	
		%		\textbf{\Huge \@title}
				
		%		\vspace{1.5cm}
		%		{\large by}\\
		%		\vspace{0.5cm}
		%		\textbf{\Large\@authors}
		%		
		%		\vspace{0.5cm}
		%		{\large \today}
		%		
		%		\vspace{1.5cm}
		%		\nicehline[1.5ex]
		%		\ifx\@supervisors\@undefined  \else
		%	 	    \large \@supervisors
		%		\fi
		%	\end{center}
		%	\vspace*{\fill}
	
		%	\varclearpage

		%	\topskip0pt
		%	\vspace*{\fill}
		%	\nicehline[1.5ex]
		%	\textsc{\begin{center} Abstract \end{center}}
			
		%	\@abstract
		%	\nicehline[1.5ex]
		%	\vspace*{\fill}
		%	\ifprint
		%		\varclearpage
		%	\fi
		%}
	}
	
	
	% Helper commands
	\newcommand{\todo}[1]{\textbf{\underline{todo} \textit{#1}}}
	
	% Math helper commands
	\newcommand{\abs}[1]{{\lvert#1\rvert}}
	\newcommand{\norm}[1]{{\lVert#1\rVert}}
\makeatother