??    .      ?  =   ?      ?  C   ?  9   5  o   o  B   ?  m   "  ?   ?  \   ?  ;   -  P   i  [   ?       @     N   Z  J   ?  D   ?  d   9  ?   ?  :   9	     t	     ?	     ?	  0   ?	     ?	  5   ?	  	   
     
  )   2
  "   \
  1   
  +   ?
  &   ?
  A     ;   F     ?  /   ?  7   ?  3   ?  :   .  ;   i  $   ?     ?     ?            2   $    W  Q   Y  F   ?  x   ?  O   k  {   ?  E   7  W   }  @   ?  X     U   o     ?  I   ?  Q     T   d  A   ?  `   ?    \  8   b     ?     ?  "   ?  0   ?       0        F     W  *   o  &   ?  /   ?  @   ?  ;   2  E   n  =   ?     ?  ;     F   B  E   ?  H   ?  Q     3   j  (   ?  )   ?     ?        +                $                                    %         )   .             ,                                !   	                '      "   (   +   -                  
                  #          &      *            --byte-subst=FORMATSTRING   substitution for unconvertible bytes
   --help                      display this help and exit
   --unicode-subst=FORMATSTRING
                              substitution for unconvertible Unicode characters
   --version                   output version information and exit
   --widechar-subst=FORMATSTRING
                              substitution for unconvertible wide characters
   -c                          discard unconvertible characters
   -f ENCODING, --from-code=ENCODING
                              the encoding of the input
   -l, --list                  list the supported encodings
   -s, --silent                suppress error messages about conversion problems
   -t ENCODING, --to-code=ENCODING
                              the encoding of the output
 %s %s argument: A format directive with a size is not allowed here. %s argument: A format directive with a variable precision is not allowed here. %s argument: A format directive with a variable width is not allowed here. %s argument: The character '%c' is not a valid conversion specifier. %s argument: The character that terminates the format directive is not a valid conversion specifier. %s argument: The format string consumes more than one argument: %u argument. %s argument: The format string consumes more than one argument: %u arguments. %s argument: The string ends in the middle of a directive. %s: I/O error %s:%u:%u %s:%u:%u: cannot convert %s:%u:%u: incomplete character or shift sequence (stdin) Converts text from one encoding to another encoding.
 I/O error Informative output:
 Options controlling conversion problems:
 Options controlling error output:
 Options controlling the input and output format:
 Report bugs to <bug-gnu-libiconv@gnu.org>.
 Try '%s --help' for more information.
 Usage: %s [OPTION...] [-f ENCODING] [-t ENCODING] [INPUTFILE...]
 Usage: iconv [-c] [-s] [-f fromcode] [-t tocode] [file ...] Written by %s.
 cannot convert byte substitution to Unicode: %s cannot convert byte substitution to target encoding: %s cannot convert byte substitution to wide string: %s cannot convert unicode substitution to target encoding: %s cannot convert widechar substitution to target encoding: %s conversion from %s to %s unsupported conversion from %s unsupported conversion to %s unsupported or:    %s -l
 or:    iconv -l try '%s -l' to get the list of supported encodings Project-Id-Version: libiconv 1.15-pre1
Report-Msgid-Bugs-To: bug-gnu-libiconv@gnu.org
PO-Revision-Date: 2016-12-12 16:30+0100
Last-Translator: Jakub Bogusz <qboosh@pld-linux.org>
Language-Team: Polish <translation-team-pl@lists.sourceforge.net>
Language: pl
MIME-Version: 1.0
Content-Type: text/plain; charset=UTF-8
Content-Transfer-Encoding: 8bit
X-Bugs: Report translation errors to the Language-Team address.
Plural-Forms: nplurals=3; plural=(n==1 ? 0 : n%10>=2 && n%10<=4 && (n%100<10 || n%100>=20) ? 1 : 2);
   --byte-subst=ŁAŃCUCH_FORMATUJĄCY   zamiennik dla niekonwertowalnych bajtów
   --help                      wyświetlenie tego opisu i zakończenie
   --unicode-subst=ŁAŃCUCH_FORMATUJĄCY
                              zamiennik dla niekonwertowalnych znaków Unicode
   --version                   wyświetlenie informacji o wersji i zakończenie
   --widechar-subst=ŁAŃCUCH_FORMATUJĄCY
                              zamiennik dla niekonwertowalnych znaków szerokich
   -c                          pominięcie niekonwertowalnych znaków
   -f KODOWANIE, --from-code=KODOWANIE
                              kodowanie wejścia
   -l, --list                  wypisanie obsługiwanych kodowań
   -s, --silent                pominięcie komunikatów błędów dotyczących konwersji
   -t KODOWANIE, --to-code=KODOWANIE
                              kodowanie wyjścia
 %s Argument %s: dyrektywa formatująca z rozmiarem nie jest tutaj dozwolona. Argument %s: dyrektywa formatująca o zmiennej precyzji nie jest tutaj dozwolona. Argument %s: dyrektywa formatująca o zmiennej szerokości nie jest tutaj dozwolona. Argument %s: znak '%c' nie jest poprawnym określeniem konwersji. Argument %s: znak kończący dyrektywę formatującą nie jest poprawnym określeniem konwersji. Argument %s: łańcuch formatujący pobiera więcej niż jeden argument: %u argument. Argument %s: łańcuch formatujący pobiera więcej niż jeden argument: %u argumenty. Argument %s: łańcuch formatujący pobiera więcej niż jeden argument: %u argumentów. Argument %s: łańcuch kończy się w środku dyrektywy. %s: błąd wej./wyj. %s:%u:%u %s:%u:%u: nie można skonwertować %s:%u:%u niedokończony znak lub sekwencja shift (stdin) Konwersja tekstu z jednego kodowania do innego.
 błąd wej./wyj. Wyjście informacyjne:
 Opcje sterujące zagadnieniami konwersji:
 Opcje sterujące wyjściem błędów:
 Opcje sterujące formatem wejścia i wyjścia:
 Błędy proszę zgłaszać na adres <bug-gnu-libiconv@gnu.org>.
 Polecenie '%s --help' pozwoli uzyskać więcej informacji.
 Składnia: %s [OPCJA...] [-f KODOWANIE] [-t KODOWANIE] [PLIK_WEJ...]
 Składnia: iconv [-c] [-s] [f z_kodu] [-t do_kodu] [plik ...] Napisane przez %s.
 nie można przekonwertować zamiennika bajtu na Unicode: %s nie można przekonwertować zamiennika bajtu na kodowanie docelowe: %s nie można przekonwertować zamiennika bajtu na łańcuch szeroki: %s nie można przekonwertować zamiennika unicode na kodowanie docelowe: %s nie można przekonwertować zamiennika znaku szerokiego na kodowanie docelowe: %s konwersja z kodu %s do kodu %s jest nieobsługiwana konwersja z kodu %s jest nieobsługiwana konwersja do kodu %s jest nieobsługiwana lub:    %s -l
 lub:    iconv -l '%s -l' poda listę obsługiwanych kodowań 