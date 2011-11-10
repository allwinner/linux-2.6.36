/** @file  unifi_wps_setup.c
 *
 * Implementation of the Wi-Fi Protected Setup (WPS) GUI.
 * 
 * @section LEGAL
 *   CONFIDENTIAL
 * 
 *   Copyright (C) Cambridge Silicon Radio Ltd 2008. All rights reserved.
 * 
 * @section DESCRIPTION
 *   This provides an implementation of Wi-Fi Protected Setup (WPS) GUI
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/unifi_apps/wps/linux/GUI/unifi_wps_setup.c#1 $
 *
 ****************************************************************************/

#include <stdio.h>
#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>

#define WPS_COMMAND "unifi_wps"
#define WPS_WALK_TIME 120

GtkWidget *entry;
GtkWidget *pin_radio;
GtkWidget *text;
GtkWidget *debug_text;
pthread_t wps_thread = 0;
pthread_t debug_thread = 0;
FILE *pStderr = NULL;
int debugWindow = 0;
int walkTime = WPS_WALK_TIME;

/* XPM */
static char * CSR_Logo_xpm[] = {
"135 135 24 1",
" 	c #000000",
".	c #0056A1",
"+	c #4080B9",
"@	c #FFFFFF",
"#	c #2B73B1",
"$	c #EAF1F7",
"%	c #B5CEE4",
"&	c #80ABD0",
"*	c #206BAD",
"=	c #4B88BD",
"-	c #A0C0DC",
";	c #C0D5E8",
">	c #D5E3F0",
",	c #E0EAF4",
"'	c #6096C4",
")	c #1665A9",
"!	c #8BB2D4",
"~	c #6B9DC8",
"{	c #367AB5",
"]	c #CADCEB",
"^	c #75A4CC",
"/	c #558EC0",
"(	c #AAC7E0",
"_	c #95B9D8",
"                                                                                                                                       ",
" ..............................................+@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ ",
" ..............................................+@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ ",
" ..............................................+@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ ",
" ..............................................+@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ ",
" ..............................................+@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ ",
" ..............................................+@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ ",
" ..............................................+@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ ",
" ..............................................+@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ ",
" ..............................................#@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ ",
" ...............................................@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ ",
" ...............................................@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ ",
" ...............................................@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ ",
" ...............................................$@@@@@@@@@@@@@@@@@@@@@@@@@@%&+#...*+=&%$@@@@@@@@%&++....#++&-;@@@@@@@@@@@@@@@@@@@@>;;; ",
" ...............................................;@@@@@@@@@@@@@@@@@@@@@@@,')............+@@@@@@-*..............&@@@@@.......&@@$!#..... ",
" ...............................................;@@@@@@@@@@@@@@@@@@@@@$~...............+@@@@${................&@@@@@.......&@%)....... ",
" ...............................................&@@@@@@@@@@@@@@@@@@@@])................+@@@@=.................&@@@@@.......&~......... ",
" ...............................................^@@@@@@@@@@@@@@@@@@@]..................+@@@]..................&@@@@@.......{.......... ",
" ...............................................+@@@@@@@@@@@@@@@@@@$)..................+@@@=........';;;%&+)..&@@@@@.................. ",
" ...............................................)@@@@@@@@@@@@@@@@@@'...........=&%;&'*.+@@@).......'@@@@@@@@>!-@@@@@.................. ",
" ................................................,@@@@@@@@@@@@@@@@$........../$@@@@@@@>!@@$........&@@@@@@@@@@@@@@@@...........#^&&&&^ ",
" ................................................%@@@@@@@@@@@@@@@@-........./@@@@@@@@@@@@@;........*$@@@@@@@@@@@@@@@.........#]@@@@@@@ ",
" ................................................^@@@@@@@@@@@@@@@@'........)$@@@@@@@@@@@@@$.........)!,@@@@@@@@@@@@@........#$@@@@@@@@ ",
" ................................................#@@@@@@@@@@@@@@@@#........^@@@@@@@@@@@@@@@*...........#'!,@@@@@@@@@........,@@@@@@@@@ ",
" .................................................$@@@@@@@@@@@@@@@.........%@@@@@@@@@@@@@@@%...............{]@@@@@@@.......#@@@@@@@@@@ ",
" .................................................-@@@@@@@@@@@@@@@.........;@@@@@@@@@@@@@@@@(................=$@@@@@.......^@@@@@@@@@@ ",
" .................................................'@@@@@@@@@@@@@@@.........;@@@@@@@@@@@@@@@@@]*...............#@@@@@.......&@@@@@@@@@@ ",
" .................................................)@@@@@@@@@@@@@@@.........;@@@@@@@@@@@@@@@@@@@%').............&@@@@.......&@@@@@@@@@@ ",
" ..................................................%@@@@@@@@@@@@@@#........!@@@@@@@@@@@@@@@@@@@@@@>!'*.........)$@@@.......&@@@@@@@@@@ ",
" ..................................................'@@@@@@@@@@@@@@'........*@@@@@@@@@@@@@@@@@@@@@@@@@@>{........%@@@.......&@@@@@@@@@@ ",
" ..................................................)@@@@@@@@@@@@@@%........./$@@@@@@@@@@@@@@@@@@@@@@@@@@*.......&@@@.......&@@@@@@@@@@ ",
" *..................................................-@@@@@@@@@@@@@@*.........*-,@@@@@@,-,@@@@@@@@@@@@@@@#.......&@@@.......&@@@@@@@@@@ ",
" ^..................................................{@@@@@@@@@@@@@@-............)++++)..;@@;#&>@@@@@@@@]........&@@@.......&@@@@@@@@@@ ",
" >...................................................,@@@@@@@@@@@@@@=...................;@@;...*'!;>@;!)........;@@@.......&@@@@@@@@@@ ",
" @*..................................................'@@@@@@@@@@@@@@$#..................;@@;...................*@@@@.......&@@@@@@@@@@ ",
" @_..................................................)$@@@@@@@@@@@@@@@/.................;@@;...................%@@@@.......&@@@@@@@@@@ ",
" @@{..................................................&@@@@@@@@@@@@@@@@%)...............;@@;..................(@@@@@.......&@@@@@@@@@@ ",
" @@$).................................................)$@@@@@@@@@@@@@@@@$&).............;@@>*...............',@@@@@@.......&@@@@@@@@@@ ",
" @@@]..................................................&@@@@@@@@@@@@@@@@@@@%^+)....#+^!;@@@@@$;&'+*....*+^-$@@@@@@@@.......&@@@@@@@@@@ ",
" @@@@&.................................................)$@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ ",
" @@@@@{.................................................&@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ ",
" @@@@@$*................................................)$@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ ",
" @@@@@@>)................................................~@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ ",
" @@@@@@@(.................................................,@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ ",
" @@@@@@@@~................................................{@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ ",
" @@@@@@@@@=................................................%@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ ",
" @@@@@@@@@$#...............................................)$@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ ",
" @@@@@@@@@@$*...............................................'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ ",
" @@@@@@@@@@@>)...............................................]@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ ",
" @@@@@@@@@@@@>)..............................................*$@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ ",
" @@@@@@@@@@@@@%...............................................=@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ ",
" @@@@@@@@@@@@@@(...............................................(@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ ",
" @@@@@@@@@@@@@@@(..............................................),@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ ",
" @@@@@@@@@@@@@@@@(..............................................#@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ ",
" @@@@@@@@@@@@@@@@@(............................................../@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ ",
" @@@@@@@@@@@@@@@@@@(..............................................(@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ ",
" @@@@@@@@@@@@@@@@@@@(.............................................)>@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ ",
" @@@@@@@@@@@@@@@@@@@@%)............................................*$@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ ",
" @@@@@@@@@@@@@@@@@@@@@>)............................................#@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ ",
" @@@@@@@@@@@@@@@@@@@@@@>*............................................/@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ ",
" @@@@@@@@@@@@@@@@@@@@@@@$#............................................&@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ ",
" @@@@@@@@@@@@@@@@@@@@@@@@$/............................................(@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ ",
" @@@@@@@@@@@@@@@@@@@@@@@@@@&............................................%@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ ",
" @@@@@@@@@@@@@@@@@@@@@@@@@@@()..........................................)>@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ ",
" @@@@@@@@@@@@@@@@@@@@@@@@@@@@>#..........................................)>@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ ",
" @@@@@@@@@@@@@@@@@@@@@@@@@@@@@$=..........................................)>@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ ",
" @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@_).........................................)>@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ ",
" @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@>#.........................................)>@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ ",
" ~@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@~.........................................)>@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ ",
" .#,@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@]*........................................)>@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ ",
" ..)%@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@$~........................................)%@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ ",
" ....~$@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@>#........................................~@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ ",
" .....#>@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@_).......................................#,@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ ",
" ......._@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@$'.......................................)_@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ ",
" ........{,@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@]{.......................................=$@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ ",
" .........)_@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@%*......................................)-@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ ",
" ...........{>@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@-*......................................{,@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ ",
" .............&$@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@-*......................................&$@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ ",
" ..............*%@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@-#.....................................*-@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ ",
" ................{,@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@]=.....................................#]@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ ",
" ..................~$@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@,^)....................................{]@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ ",
" ...................)&$@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@$-{....................................{]@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ ",
" .....................*-@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@,^*...................................#-@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ ",
" .......................*-@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@>^*..................................)&,@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ ",
" .........................*-@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@>^*..................................=-$@@@@@@@@@@@@@@@@@@@@@@@@@@@ ",
" ...........................*&$@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@,!=.................................)'%@@@@@@@@@@@@@@@@@@@@@@@@@ ",
" .............................)^,@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@;&+.................................=-,@@@@@@@@@@@@@@@@@@@@@ ",
" ................................=%@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@;!'*...............................*'->@@@@@@@@@@@@@@@@@ ",
" ..................................*&,@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@$;!'+..............................)+^!;$@@@@@@@@@@@ ",
" .....................................=-@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@$;-&'+*.............................#+'&&&;;;@ ",
" .......................................)'%@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@;;;&&&&=+++++++++++++++++&&&&!;;;$%!_ ",
" ..........................................)'%@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@>!=)&;_ ",
" ..............................................=!,@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@,-').)&$(+- ",
" .................................................*'%$@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@>!=)...)&$@()># ",
" .....................................................*'->@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@,%&+......)&$@@/._]. ",
" ..........................................................+^-;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@;-&+)........)&$@@${.=@'. ",
" ...............................................................*+^&;;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@>;!&+*............)&$@@@>#.*$,.. ",
" .......................................................................#++&&&-;;;;;;;;;;;;;;;-&&^++).................*-$@@@@])..]@^.. ",
" ~..................................................................................................................{-@@@@@@&...&@$).. ",
" @]*..............................................................................................................'>@@@@@@$=...=@@&... ",
" @@@&).........................................................................................................*&$@@@@@@@>*...*$@@*... ",
" @@@@$'......................................................................................................=%@@@@@@@@@_.....>@@-.... ",
" @@@@@@,'.................................................................................................*&,@@@@@@@@@$=.....(@@@*.... ",
" @@@@@@@@,'............................................................................................)']@@@@@@@@@@@])...../@@@-..... ",
" @@@@@@@@@@,'........................................................................................=%@@@@@@@@@@@@$~......#@@@@*..... ",
" @@@@@@@@@@@@$&*..................................................................................=-$@@@@@@@@@@@@@]*......*$@@@-...... ",
" @@@@@@@@@@@@@@@%=............................................................................)'%$@@@@@@@@@@@@@@$~.......)>@@@@*...... ",
" @@@@@@@@@@@@@@@@@$!#......................................................................#&>@@@@@@@@@@@@@@@@@]*........%@@@@-....... ",
" @@@@@@@@@@@@@@@@@@@@,!=...............................................................*'-$@@@@@@@@@@@@@@@@@@$'.........(@@@@$*....... ",
" @@@@@@@@@@@@@@@@@@@@@@@$%^*.......................................................#^-,@@@@@@@@@@@@@@@@@@@@@_).........~@@@@@&........ ",
" @@@@@@@@@@@@@@@@@@@@@@@@@@@,%^+..............................................#'!;@@@@@@@@@@@@@@@@@@@@@@@@]#........../@@@@@$)........ ",
" @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@>-&+#..................................+=&->@@@@@@@@@@@@@@@@@@@@@@@@@@@,=...........{@@@@@@~......... ",
" @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@,;-&&=++*.............)+++'&&;;$@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@$&............#$@@@@@,.......... ",
" @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@$&)............#$@@@@@@{.......... ",
" @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@$&).............#$@@@@@@%........... ",
" @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@$&)..............#$@@@@@@$*........... ",
" @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@,&)...............#$@@@@@@@~............ ",
" @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@>'.................#$@@@@@@@>............. ",
" @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@-{..................#$@@@@@@@@#............. ",
" @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@,^)...................#$@@@@@@@@&.............. ",
" @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@$-#.....................#$@@@@@@@@>............... ",
" @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@$-=......................./@@@@@@@@@$#............... ",
" ^>@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@,-=........................./@@@@@@@@@@~................ ",
" ..)=-,@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@;&#...........................(@@@@@@@@@@%................. ",
" ......*'->@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@;!=)............................)%@@@@@@@@@@,)................. ",
" ..........)+&%,@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@,%&+................................*>@@@@@@@@@@@#.................. ",
" ................+'&;>@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@,;!^+....................................{$@@@@@@@@@@@/................... ",
" ......................)+=&&%;;$@@@@@@@@@@@@@@@@@@@;;;!&'+#.........................................&@@@@@@@@@@@@(.................... ",
" ...................................#+++++++++)...................................................)]@@@@@@@@@@@@>..................... ",
" ................................................................................................{$@@@@@@@@@@@@>)..................... ",
" ..............................................................................................)_@@@@@@@@@@@@@$*...................... ",
" .............................................................................................#>@@@@@@@@@@@@@$#....................... ",
" ...........................................................................................)&@@@@@@@@@@@@@@$#........................ ",
" ..........................................................................................',@@@@@@@@@@@@@@$#......................... ",
"                                                                                                                                       "};

/* This variation of popen returns the child pid and only supports reading of the child output by the parent. *
 * It also creates two pipes so that stdout and stderr can be handled separately                              */
pid_t my_popen( char *command, FILE **ppStdout, FILE **ppStderr ) 
{ 
int stdout_pipe[2];
int stderr_pipe[2]; 
pid_t pid;

    if( pipe( stdout_pipe ) < 0 )
    {
        return 0;
    }
     
    if( pipe( stderr_pipe ) < 0 )
    {
        close( stdout_pipe[0] ); 
        close( stdout_pipe[1] ); 
        return 0;
    }

    if( ( pid = fork() ) > 0 )
    {
        /* Parent process */ 
        close( stdout_pipe[1] );
        *ppStdout = fdopen( stdout_pipe[0], "r" ); 
        close( stderr_pipe[1] ); 
        *ppStderr = fdopen( stderr_pipe[0], "r" ); 
        return pid; 
    }
    else
    if( pid == 0 )
    {
        /* Child process */ 
        setpgid( 0, 0 ); 
        fflush( stdout ); 
        fflush( stderr );
        dup2( stdout_pipe[1], STDOUT_FILENO );
        dup2( stderr_pipe[1], STDERR_FILENO );
        close( stdout_pipe[0] ); 
        close( stdout_pipe[1] ); 
        close( stderr_pipe[0] ); 
        close( stderr_pipe[1] ); 
        // replace this process with the requested command
        execl( "/bin/sh", "sh", "-c", command, NULL );
    }
    else
    {
        /* Fork error */
        close( stdout_pipe[0] ); 
        close( stdout_pipe[1] ); 
        close( stderr_pipe[0] ); 
        close( stderr_pipe[1] ); 
    } 

    return 0; 
}

void generatePin( GtkWidget *entry )
{
FILE *pStdout;
char buffer[100] = "";
char *pCh;
pid_t pid;
int status;

    if( ( pid = my_popen( WPS_COMMAND " -g", &pStdout, &pStderr ) ) )
    {
        while( fgets( buffer, 80, pStdout ) != NULL ) /* void */;
        pclose( pStdout );
        while( ( pCh = strrchr( buffer, '\n' ) ) != NULL ) *pCh = '\0';
        gtk_entry_set_text (GTK_ENTRY (entry), buffer );
        wait( &status );
    }
}

void *wps_start( void *args )
{
FILE *pStdout;
char command[100];
char buffer[100];
GdkColor red;
GdkColor green;
GdkColor blue;
GdkColor *pColour;
GdkFont *pFont;
pid_t pid;    

    gdk_threads_enter ();
    pFont = gdk_font_load ("-adobe-helvetica-bold-r-*-*-*-140-*-*-*-*-*-*");

    gdk_color_parse( "red", &red );
    gdk_color_parse( "darkgreen", &green );
    gdk_color_parse( "darkblue", &blue );

    gtk_text_backward_delete( (GtkText *)text, gtk_text_get_length( (GtkText *)text ) );

    if (GTK_TOGGLE_BUTTON(pin_radio)->active)
    {
        /* PIN mode comamnd line parameters */
        sprintf( command, WPS_COMMAND " -p %s -t %d%s", gtk_entry_get_text(GTK_ENTRY(entry)), walkTime, debugWindow ? " -d" : ""  );
    }
    else
    {
        /* Pushbutton mode comamnd line parameters */
        sprintf( command, WPS_COMMAND " -t %d%s", walkTime, debugWindow ? " -d" : "" );
    }
    gdk_threads_leave ();

    pid = my_popen( command, &pStdout, &pStderr );

    while( fgets( buffer, 80, pStdout ) != NULL )
    {
        if( strstr( buffer, "successful" ) )
        {
            pColour = &green;
        }
        else
        if( strstr( buffer, "failed" ) || strstr( buffer, "expired" ) || strstr( buffer, "Overlap" ) )
        {
            pColour = &red;
        }
        else
        {
            pColour = &blue;
        }
        gdk_threads_enter ();
        gtk_text_insert (GTK_TEXT (text), pFont, pColour, NULL, buffer, -1);
        gdk_threads_leave ();
    }

    pclose( pStdout );

    /* Reset the activation button label when done */
    gdk_threads_enter ();
    gtk_label_set_text( GTK_LABEL( args ), "OK" );
    gdk_threads_leave ();

    wps_thread = 0;

    return NULL;
}

void wps_action(GtkWidget *button, GtkWidget *label)
{
int status;

    if( !wps_thread )
    {
        /* Kill any old command line processes that might have been left hanging */
        system( "pkill -SIGINT -x " WPS_COMMAND );
        pthread_create( &wps_thread, NULL, wps_start, label );
        gtk_label_set_text( GTK_LABEL( label ), "Abort" );
    }
    else
    {
        /* Kill the WPS command line - this will cause the thread to exit in wps_start() */
        system( "pkill -SIGINT -x " WPS_COMMAND );
        gtk_text_backward_delete( (GtkText *)text, gtk_text_get_length( (GtkText *)text ) );
        gtk_label_set_text( GTK_LABEL( label ), "OK" );
    }
    wait( &status );
}

void newPin_action (GtkWidget *button, GtkWidget *entry)
{
    generatePin( entry );
}

void close_application( GtkWidget *widget, gpointer data )
{
    if( wps_thread )
    {
        system("pkill -SIGINT -x " WPS_COMMAND );
    }
    gtk_main_quit();
}

static void walk_time( void *arg )
{
    if( walkTime )
    {
        walkTime = 0;
    }
    else
    {
        walkTime = WPS_WALK_TIME;
    }
}

void *wps_debug( void *args )
{
char buffer[100];
GdkFont *fixed_font;
    
    gdk_threads_enter ();
    fixed_font = gdk_font_load ("-*-courier-medium-r-*-*-*-120-*-*-*-*-*-*");
    gdk_threads_leave ();
    
    while( debugWindow )
    {
        /* Only try to read pStderr when the wps thread is active */
        if( wps_thread )
        {
            while( fgets( buffer, 80, pStderr ) != NULL && debugWindow )
            {
                gdk_threads_enter ();
                gtk_text_insert( GTK_TEXT( debug_text ), fixed_font, &debug_text->style->black, NULL, buffer, -1 );
                gdk_threads_leave ();
            }
        }
    }

    debug_thread = 0;
    
    return NULL;
}

void file_ok( GtkWidget *widget, GtkFileSelection *selector )
{
gchar *pBuffer;
const gchar *pFileName;
int fd;
    
    pFileName = gtk_file_selection_get_filename( GTK_FILE_SELECTION( selector ) );

    fd = open( pFileName, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH );
    if( fd > 0 )
    {    
    guint length;
    
        length = gtk_text_get_length( GTK_TEXT( debug_text ) );
        pBuffer = gtk_editable_get_chars( GTK_EDITABLE( debug_text ), 0, -1 );
        write( fd, pBuffer, length );
        g_free( pBuffer );
        close( fd );
        gtk_widget_destroy( GTK_WIDGET( selector ) );
    }
    else
    {
    GtkWidget *dialog;
    GtkWidget *button;
    GtkWidget *label;
        
        dialog = gtk_dialog_new();
        gtk_window_set_title( GTK_WINDOW( dialog ), "ERROR");
        gtk_widget_set_usize( dialog, 200, 100 );
        
        button = gtk_button_new_with_label( "OK" );

        /* Simply destroy the dialog when the OK is clicked */
        gtk_signal_connect_object( GTK_OBJECT( button ), "clicked",
                                   (GtkSignalFunc)gtk_widget_destroy, GTK_OBJECT( dialog ) );
                            
        gtk_box_pack_start( GTK_BOX( GTK_DIALOG( dialog )->action_area ),
                            button, TRUE, TRUE, 0 );
        gtk_widget_show( button );
        
        label = gtk_label_new( "Unable to open file" );
        gtk_box_pack_start( GTK_BOX( GTK_DIALOG( dialog )->vbox ),
                            label, TRUE, TRUE, 0 );
        gtk_widget_show( label );
        gtk_widget_show( dialog );
    }
}

static void save_file( void *arg )
{
GtkWidget *selector;
    
    selector = gtk_file_selection_new ("File selection");
    
    /* Connect the OK button to file_ok function */
    gtk_signal_connect( GTK_OBJECT( GTK_FILE_SELECTION( selector )->ok_button ),
                        "clicked", (GtkSignalFunc)file_ok, selector );
    
    /* Connect the Cancel button to destroy the widget */
    gtk_signal_connect_object( GTK_OBJECT( GTK_FILE_SELECTION( selector )->cancel_button),
                               "clicked", (GtkSignalFunc)gtk_widget_destroy, GTK_OBJECT( selector ) );

    /* Select a default filename */
    gtk_file_selection_set_filename( GTK_FILE_SELECTION( selector ), "wps.log" );
                               
    gtk_widget_show( selector );   
}

static void clear_debug_window( void *arg )
{
    gtk_text_backward_delete( (GtkText *)debug_text, gtk_text_get_length( (GtkText *)debug_text ) );
}

void close_debug( GtkWidget *widget, gpointer window )
{
    debugWindow = 0;
}

static void debug_window( void *arg )
{
GtkWidget *window;
GtkWidget *box;
GtkWidget *vscrollbar;
GtkWidget *table;
GtkWidget *file_menu;
GtkWidget *edit_menu;
GtkWidget *menu;
GtkWidget *menu_bar;
GtkWidget *menu_items;

    if( !debugWindow )
    {
       /* Create the debug window */ 
       window = gtk_window_new( GTK_WINDOW_TOPLEVEL );

#ifdef GTK_ENABLE_BROKEN
       gtk_window_set_type_hint( GTK_WINDOW( window ), GDK_WINDOW_TYPE_HINT_DIALOG );
       gtk_window_set_default_size( GTK_WINDOW( window ), 550, 210 );
       gtk_widget_set_size_request( window, 0, 0 );
#else
       gtk_widget_set_usize( window, 550, 210 );
       gtk_window_set_policy( GTK_WINDOW( window ), TRUE, TRUE, FALSE );
#endif

       gtk_signal_connect( GTK_OBJECT( window ), "destroy",
                           GTK_SIGNAL_FUNC( close_debug ),
                           NULL);
       gtk_window_set_title( GTK_WINDOW( window ), "Wi-Fi Protected Setup debug");
       gtk_container_border_width( GTK_CONTAINER( window ), 0);
       gtk_widget_show( window );

       /* Add a packing box */
       box = gtk_vbox_new( FALSE, 0 );
       gtk_container_add( GTK_CONTAINER( window ), box );
       gtk_widget_show( box );

       /* Add the menu bar first */
       menu_bar = gtk_menu_bar_new ();
       gtk_box_pack_start( GTK_BOX( box ), menu_bar, FALSE, FALSE, 2 );
       gtk_widget_show( menu_bar );

       file_menu = gtk_menu_new ();
       menu = gtk_menu_item_new_with_label( "File" );       
       gtk_widget_show( menu );
       gtk_menu_item_set_submenu( GTK_MENU_ITEM( menu ), file_menu );
       gtk_menu_bar_append( GTK_MENU_BAR( menu_bar ), menu );
       menu_items = gtk_menu_item_new_with_label( "Save..." );
       gtk_menu_append( GTK_MENU( file_menu ), menu_items );
       gtk_signal_connect_object( GTK_OBJECT( menu_items ), "activate",
                    GTK_SIGNAL_FUNC( save_file ), NULL );       
       gtk_widget_show( menu_items );
       menu_items = gtk_menu_item_new_with_label( "Quit" );
       gtk_menu_append( GTK_MENU( file_menu ), menu_items );
       gtk_signal_connect_object( GTK_OBJECT( menu_items ), "activate",
                         (GtkSignalFunc)gtk_widget_destroy, GTK_OBJECT( window ) );       
       gtk_widget_show( menu_items );

       edit_menu = gtk_menu_new ();
       menu = gtk_menu_item_new_with_label( "Edit" );       
       gtk_widget_show( menu );
       gtk_menu_item_set_submenu( GTK_MENU_ITEM( menu ), edit_menu );
       gtk_menu_bar_append( GTK_MENU_BAR( menu_bar ), menu );
       menu_items = gtk_menu_item_new_with_label( "Clear window" );
       gtk_menu_append( GTK_MENU( edit_menu ), menu_items );
       gtk_signal_connect_object( GTK_OBJECT( menu_items ), "activate",
                    GTK_SIGNAL_FUNC( clear_debug_window ), NULL );       
       gtk_widget_show( menu_items );

       /* Add a table for the text window and scrollbar */
       table = gtk_table_new( 2, 2, FALSE );
       gtk_table_set_row_spacing( GTK_TABLE( table ), 0, 2 );
       gtk_table_set_col_spacing( GTK_TABLE( table ), 0, 2 );
       gtk_box_pack_start( GTK_BOX( box ), table, TRUE, TRUE, 0 );
       gtk_widget_show( table );

       /* Add the text widget */
       debug_text = gtk_text_new( NULL, NULL );
       gtk_text_set_editable( GTK_TEXT( debug_text), FALSE );
       gtk_table_attach( GTK_TABLE( table ), debug_text, 0, 1, 0, 1,
                         GTK_EXPAND | GTK_SHRINK | GTK_FILL,
                         GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0 );
       gtk_widget_show( debug_text );
       
       /* Add the scrollbar */
       vscrollbar = gtk_vscrollbar_new( GTK_TEXT( debug_text )->vadj );
       gtk_table_attach( GTK_TABLE( table ), vscrollbar, 1, 2, 0, 1,
                         GTK_FILL, GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0 );
       gtk_widget_show( vscrollbar );
       
       gtk_widget_show( window );
                               
       debugWindow = 1;
       
       pthread_create( &debug_thread, NULL, wps_debug, NULL );
    }
}

static void menu_delete_event( void *arg )
{
    gtk_widget_destroy( GTK_WIDGET( arg ) );
}

static gint button_press( GtkWidget *widget,
                          GdkEvent *event )
{
GtkWidget *menu;
GtkWidget *menu_items;

    if (event->type == GDK_BUTTON_PRESS) {

        GdkEventButton *bevent = (GdkEventButton *) event;
        /* Require left button press to activate menu */ 
        if( bevent->button == 1 )
        { 
           menu = gtk_menu_new ();

           gtk_signal_connect (GTK_OBJECT(menu), "destroy",
                           GTK_SIGNAL_FUNC(menu_delete_event), menu );
 
           if( !debugWindow )
           {
               menu_items = gtk_menu_item_new_with_label( "Open debug window" );
               gtk_menu_append (GTK_MENU (menu), menu_items);
               gtk_signal_connect_object (GTK_OBJECT (menu_items), "activate",
                       GTK_SIGNAL_FUNC( debug_window ), NULL );
               gtk_widget_show (menu_items);
           }

           if( walkTime )
           {
               menu_items = gtk_menu_item_new_with_label( "Disable walk time" );
           }
           else
           {
               menu_items = gtk_menu_item_new_with_label( "Enable walk time" );
           }
                      
           gtk_menu_append (GTK_MENU (menu), menu_items);
           gtk_signal_connect_object (GTK_OBJECT (menu_items), "activate",
                   GTK_SIGNAL_FUNC( walk_time ), NULL );
           gtk_widget_show (menu_items);

        
           gtk_menu_popup (GTK_MENU(menu), NULL, NULL, NULL, NULL,
                           bevent->button, bevent->time);
                        
           /* Tell calling code that we have handled this event */
           return TRUE;
        }
    }

    /* Tell calling code that we have not handled this event */
    return FALSE;
}

int main (int argc, char *argv[])
{
  GtkWidget *window;
  GtkWidget *box1;
  GtkWidget *table;
  GtkWidget *button;
  GtkWidget *label;
  GSList *group;  
  GdkPixmap *pixmap;
  GtkWidget *pixmapwid;
  GdkBitmap *mask;
  GtkStyle *style;
  GtkWidget *radio;
  GtkWidget *menu_button;
  
  g_thread_init (NULL);

#ifdef GTK_ENABLE_BROKEN
  gdk_threads_init();
#endif

  gtk_init (&argc, &argv);

  window = gtk_window_new( GTK_WINDOW_TOPLEVEL );
  
#ifdef GTK_ENABLE_BROKEN
  gtk_window_set_type_hint( GTK_WINDOW( window ), GDK_WINDOW_TYPE_HINT_DIALOG );
  gtk_window_set_default_size( GTK_WINDOW( window ), 350, 210 );
  gtk_widget_set_size_request( window, 0, 0 );
#else
  gtk_widget_set_usize( window, 350, 210 );
  gtk_window_set_policy( GTK_WINDOW( window ), TRUE, TRUE, FALSE );
#endif
  
  gtk_signal_connect( GTK_OBJECT( window ), "destroy",
                      GTK_SIGNAL_FUNC( close_application ),
                      NULL );
  gtk_window_set_title( GTK_WINDOW( window ), "Wi-Fi Protected Setup" );
  gtk_container_border_width( GTK_CONTAINER( window ), 0 );
  gtk_widget_show( window );

  box1 = gtk_vbox_new( FALSE, 0 );
  gtk_container_add( GTK_CONTAINER( window ), box1 );
  gtk_widget_show( box1 );
  
  table = gtk_table_new( 6, 21, TRUE );
  gtk_box_pack_start( GTK_BOX( box1 ), table, TRUE, TRUE, 0 );

  /* Create a button for the options menu */
  menu_button = gtk_button_new();
  gtk_table_attach_defaults( GTK_TABLE( table ), menu_button, 0, 9, 2, 6 );
  gtk_widget_show( menu_button );
  
  /* Add the CSR logo to the menu button to hide it */
  style = gtk_widget_get_default_style();
  pixmap = gdk_pixmap_create_from_xpm_d( window->window, &mask,
                                         &style->bg[GTK_STATE_NORMAL],
                                         CSR_Logo_xpm );
  pixmapwid = gtk_pixmap_new( pixmap, mask );
  gtk_container_add (GTK_CONTAINER( menu_button ), pixmapwid );
  gtk_widget_show( pixmapwid );

  gtk_signal_connect_object( GTK_OBJECT( menu_button ), "event",
      GTK_SIGNAL_FUNC( button_press ), NULL );
        
  /* Create the GtkText widget */
  text = gtk_text_new( NULL, NULL );
  gtk_text_set_editable( GTK_TEXT( text ), FALSE );
  gtk_table_attach_defaults( GTK_TABLE( table ), text, 0, 21, 0, 2 );
  gtk_widget_show( text );

  entry = gtk_entry_new();
  gtk_entry_set_editable( GTK_ENTRY( entry ), FALSE );
  gtk_table_attach_defaults( GTK_TABLE( table ), entry, 9, 14, 2, 3 );
  GTK_WIDGET_SET_FLAGS( entry, GTK_CAN_DEFAULT );
  generatePin( entry );
  gtk_widget_show( entry );

  button = gtk_button_new_with_label( "New PIN" );
  gtk_table_attach_defaults( GTK_TABLE( table ), button, 15, 21, 2, 3 );
  GTK_WIDGET_SET_FLAGS( button, GTK_CAN_DEFAULT );
  gtk_signal_connect( GTK_OBJECT( button ), "clicked",
                      GTK_SIGNAL_FUNC( newPin_action ), entry );
  gtk_widget_show( button );

  pin_radio = gtk_radio_button_new_with_label( NULL, "PIN" );
  gtk_table_attach_defaults( GTK_TABLE( table ), pin_radio, 9, 15, 3, 4 );
  gtk_widget_show( pin_radio );

  group = gtk_radio_button_group( GTK_RADIO_BUTTON( pin_radio ) );
  radio = gtk_radio_button_new_with_label( group, "Pushbutton" );
  gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( radio ), TRUE );
  gtk_table_attach_defaults( GTK_TABLE( table ), radio, 9, 15, 4, 5 );
  gtk_widget_show( radio );

  button = gtk_button_new_with_label( "Quit" );
  gtk_signal_connect( GTK_OBJECT( button ), "clicked",
                      GTK_SIGNAL_FUNC( close_application ),
                      NULL );
  gtk_table_attach_defaults( GTK_TABLE( table ), button, 9, 15, 5, 6 );
  GTK_WIDGET_SET_FLAGS( button, GTK_CAN_DEFAULT );
  gtk_widget_grab_default( button );
  gtk_widget_show( button );

  label = gtk_label_new( "OK" );
  gtk_widget_show( label );
  button = gtk_button_new();
  gtk_container_add( GTK_CONTAINER( button ), label );  

  gtk_table_attach_defaults( GTK_TABLE( table ), button, 15, 21, 5, 6 );
  GTK_WIDGET_SET_FLAGS( button, GTK_CAN_DEFAULT );
  gtk_signal_connect( GTK_OBJECT( button ), "clicked",
                      GTK_SIGNAL_FUNC( wps_action ), label );
  gtk_widget_show( button );

  gtk_widget_show( table );

  gdk_threads_enter();
  gtk_main();
  gdk_threads_leave();

  return 0;       
}


