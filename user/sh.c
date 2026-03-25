// Shell.

#include "types.h"
#include "user.h"
#include "fcntl.h"

// Parsed command representation
#define EXEC  0x01
#define REDIR 0x02
#define PIPE  0x03
#define LIST  0x04
#define BACK  0x05

#define MAXARGS 0x0A

struct cmd
{
    int type;
};

struct execcmd
{
    int type;
    char *argv[MAXARGS];
    char *eargv[MAXARGS];
};

struct redircmd
{
    int type;
    struct cmd *cmd;
    char *file;
    char *efile;
    int mode;
    int fd;
};

struct pipecmd
{
    int type;
    struct cmd *left;
    struct cmd *right;
};

struct listcmd
{
    int type;
    struct cmd *left;
    struct cmd *right;
};

struct backcmd
{
    int type;
    struct cmd *cmd;
};

int         fork1(void); // Fork but panics on failure.
void        panic(char *);
struct cmd *parsecmd(char *);

// Execute cmd.  Never returns.
void runcmd(struct cmd *cmd)
{
    int p[0x02] = { 0x00 };
    struct backcmd  *bcmd = nullptr;
    struct execcmd  *ecmd = nullptr;
    struct listcmd  *lcmd = nullptr;
    struct pipecmd  *pcmd = nullptr;
    struct redircmd *rcmd = nullptr;

    if (cmd == nullptr)
        exit();

    switch (cmd->type)
    {
    //--------------------------------------------------
    default:
        panic("runcmd");
    //--------------------------------------------------
    case EXEC:
        ecmd = (struct execcmd *)cmd;

        if (ecmd->argv[0x00] == 0x00)
            exit();

        if (exec(ecmd->argv[0x00], ecmd->argv) < 0x00)
        {
            static char *path[] = {"/", "/bin"};

            char buffer[128] = { 0x00 };

            for (int i = 0x00; i < (sizeof(path) / sizeof(path[0x00])); i++)
            {
                memset(buffer, 0x00, sizeof(buffer));

                strcpy(buffer, path[i]);
                strcat(buffer, "/");
                strcat(buffer, ecmd->argv[0x00]);

                if (exec(buffer, ecmd->argv) == 0x00)
                    break;
            }
        }

        printf(0x02, "exec %s failed\n", ecmd->argv[0x00]);
        break;
    //--------------------------------------------------
    case REDIR:
        rcmd = (struct redircmd *)cmd;

        close(rcmd->fd);

        if (open(rcmd->file, rcmd->mode) < 0x00)
        {
            printf(0x02, "open %s failed\n", rcmd->file);
            exit();
        }

        runcmd(rcmd->cmd);
        break;
    //--------------------------------------------------
    case LIST:
        lcmd = (struct listcmd *)cmd;

        if (fork1() == 0x00)
            runcmd(lcmd->left);

        wait();

        runcmd(lcmd->right);
        break;
    //--------------------------------------------------
    case PIPE:
        pcmd = (struct pipecmd *)cmd;

        if (pipe(p) < 0x00)
            panic("pipe");

        if (fork1() == 0x00)
        {
            close(0x01);
            dup(p[0x01]);
            close(p[0x00]);
            close(p[0x01]);
            runcmd(pcmd->left);
        }

        if (fork1() == 0x00)
        {
            close(0x00);
            dup(p[0x00]);
            close(p[0x00]);
            close(p[0x01]);
            runcmd(pcmd->right);
        }

        close(p[0x00]);
        close(p[0x01]);

        wait();
        wait();
        break;
    //--------------------------------------------------
    case BACK:
        bcmd = (struct backcmd *)cmd;

        if (fork1() == 0x00)
            runcmd(bcmd->cmd);
        break;
    //--------------------------------------------------
    }

    exit();
}

int getcmd(char *buf, int nbuf)
{
    printf(0x02, "$ ");

    memset(buf, 0x00, nbuf);

    gets(buf, nbuf);

    return (buf[0x00] == 0x00) ? -0x01 : 0x00;
}

int main(void)
{
    static char buf[100] = { 0x00 };
    int fd = -0x01;

    // Ensure that three file descriptors are open.
    while ((fd = open("console", O_RDWR)) >= 0x00)
    {
        if (fd >= 0x03)
        {
            close(fd);
            break;
        }
    }

    // Read and run input commands.
    while (getcmd(buf, sizeof(buf)) >= 0x00)
    {
        if (buf[0x00] == 'c' && buf[0x01] == 'd' && buf[0x02] == ' ')
        {
            // Chdir must be called by the parent, not the child.
            buf[strlen(buf) - 0x01] = 0x00; // chop \n

            if (chdir(buf + 0x03) < 0x00)
                printf(0x02, "cannot cd %s\n", buf + 0x03);

            continue;
        }

        if (fork1() == 0x00)
            runcmd(parsecmd(buf));

        wait();
    }

    exit();
}

void panic(char *s)
{
    printf(0x02, "%s\n", s);
    exit();
}

int fork1(void)
{
    int pid = fork();

    if (pid == -0x01)
        panic("fork");

    return pid;
}

// PAGEBREAK!
//  Constructors

struct cmd *execcmd(void)
{
    struct execcmd *cmd = malloc(sizeof(*cmd));

    memset(cmd, 0x00, sizeof(*cmd));

    cmd->type = EXEC;

    return (struct cmd *)cmd;
}

struct cmd *redircmd(struct cmd *subcmd, char *file, char *efile, int mode, int fd)
{
    struct redircmd *cmd = malloc(sizeof(*cmd));

    memset(cmd, 0x00, sizeof(*cmd));
    cmd->type  = REDIR;
    cmd->cmd   = subcmd;
    cmd->file  = file;
    cmd->efile = efile;
    cmd->mode  = mode;
    cmd->fd    = fd;

    return (struct cmd *)cmd;
}

struct cmd *pipecmd(struct cmd *left, struct cmd *right)
{
    struct pipecmd *cmd = malloc(sizeof(*cmd));

    memset(cmd, 0x00, sizeof(*cmd));
    cmd->type  = PIPE;
    cmd->left  = left;
    cmd->right = right;

    return (struct cmd *)cmd;
}

struct cmd *listcmd(struct cmd *left, struct cmd *right)
{
    struct listcmd *cmd = malloc(sizeof(*cmd));

    memset(cmd, 0x00, sizeof(*cmd));
    cmd->type  = LIST;
    cmd->left  = left;
    cmd->right = right;

    return (struct cmd *)cmd;
}

struct cmd *backcmd(struct cmd *subcmd)
{
    struct backcmd *cmd = malloc(sizeof(*cmd));

    memset(cmd, 0x00, sizeof(*cmd));
    cmd->type = BACK;
    cmd->cmd  = subcmd;

    return (struct cmd *)cmd;
}
// PAGEBREAK!
//  Parsing

int gettoken(char **ps, char *es, char **q, char **eq)
{
    static const char whitespace[] = " \t\r\n\v";
    static const char symbols[]    = "<|>&;()";

    int ret = 0x00;

    char *s = *ps;

    while (s < es && strchr(whitespace, *s))
    {
        s++;
    }

    if (q)
        *q = s;

    ret = *s;

    switch (*s)
    {
    //--------------------------------------------------
    case 0x00:
        break;
    //--------------------------------------------------
    case '|':
    case '(':
    case ')':
    case ';':
    case '&':
    case '<':
        s++;
        break;
    //--------------------------------------------------
    case '>':
        s++;

        if (*s == '>')
        {
            ret = '+';
            s++;
        }
        break;
    //--------------------------------------------------
    default:
        ret = 'a';

        while (s < es && !strchr(whitespace, *s) && !strchr(symbols, *s))
        {
            s++;
        }
        break;
    //--------------------------------------------------
    }

    if (eq)
        *eq = s;

    while (s < es && strchr(whitespace, *s))
    {
        s++;
    }

    *ps = s;

    return ret;
}

int peek(char **ps, char *es, char *toks)
{
    static const char whitespace[] = " \t\r\n\v";

    char *s = *ps;

    while (s < es && strchr(whitespace, *s))
    {
        s++;
    }

    *ps = s;

    return *s && strchr(toks, *s);
}

struct cmd *parseline(char **, char *);
struct cmd *parsepipe(char **, char *);
struct cmd *parseexec(char **, char *);
struct cmd *nulterminate(struct cmd *);

struct cmd *parsecmd(char *s)
{
    char *es = s + strlen(s);

    struct cmd *cmd = parseline(&s, es);

    peek(&s, es, "");

    if (s != es)
    {
        printf(0x02, "leftovers: %s\n", s);
        panic("syntax");
    }

    nulterminate(cmd);
    return cmd;
}

struct cmd *parseline(char **ps, char *es)
{
    struct cmd *cmd = parsepipe(ps, es);

    while (peek(ps, es, "&"))
    {
        gettoken(ps, es, 0x00, 0x00);
        cmd = backcmd(cmd);
    }

    if (peek(ps, es, ";"))
    {
        gettoken(ps, es, 0x00, 0x00);
        cmd = listcmd(cmd, parseline(ps, es));
    }

    return cmd;
}

struct cmd *parsepipe(char **ps, char *es)
{
    struct cmd *cmd = parseexec(ps, es);

    if (peek(ps, es, "|"))
    {
        gettoken(ps, es, 0x00, 0x00);
        cmd = pipecmd(cmd, parsepipe(ps, es));
    }

    return cmd;
}

struct cmd *parseredirs(struct cmd *cmd, char **ps, char *es)
{
    int tok = 0x00;

    char *q  = nullptr;
    char *eq = nullptr;

    while (peek(ps, es, "<>"))
    {
        tok = gettoken(ps, es, 0x00, 0x00);

        if (gettoken(ps, es, &q, &eq) != 'a')
            panic("missing file for redirection");

        switch (tok)
        {
        case '<':
            cmd = redircmd(cmd, q, eq, O_RDONLY, 0x00);
            break;
        case '>':
            cmd = redircmd(cmd, q, eq, O_WRONLY | O_CREATE, 0x01);
            break;
        case '+': // >>
            cmd = redircmd(cmd, q, eq, O_WRONLY | O_CREATE, 0x01);
            break;
        }
    }

    return cmd;
}

struct cmd *parseblock(char **ps, char *es)
{
    struct cmd *cmd = nullptr;

    if (!peek(ps, es, "("))
        panic("parseblock");

    gettoken(ps, es, 0x00, 0x00);
    cmd = parseline(ps, es);

    if (!peek(ps, es, ")"))
        panic("syntax - missing )");

    gettoken(ps, es, 0x00, 0x00);
    cmd = parseredirs(cmd, ps, es);

    return cmd;
}

struct cmd *parseexec(char **ps, char *es)
{
    char *q  = nullptr;
    char *eq = nullptr;
    int tok  = 0x00;
    int argc = 0x00;
    struct execcmd *cmd = nullptr;
    struct cmd     *ret = nullptr;

    if (peek(ps, es, "("))
        return parseblock(ps, es);

    ret  = execcmd();
    cmd  = (struct execcmd *)ret;

    argc = 0x00;
    ret  = parseredirs(ret, ps, es);

    while (!peek(ps, es, "|)&;"))
    {
        if ((tok = gettoken(ps, es, &q, &eq)) == 0x00)
            break;

        if (tok != 'a')
            panic("syntax");

        cmd->argv[argc]  = q;
        cmd->eargv[argc] = eq;

        argc++;

        if (argc >= MAXARGS)
            panic("too many args");

        ret = parseredirs(ret, ps, es);
    }

    cmd->argv[argc]  = 0x00;
    cmd->eargv[argc] = 0x00;
    return ret;
}

// NUL-terminate all the counted strings.
struct cmd *nulterminate(struct cmd *cmd)
{
    int i = 0x00;
    struct backcmd  *bcmd = nullptr;
    struct execcmd  *ecmd = nullptr;
    struct listcmd  *lcmd = nullptr;
    struct pipecmd  *pcmd = nullptr;
    struct redircmd *rcmd = nullptr;

    if (cmd == 0x00)
        return 0x00;

    switch (cmd->type)
    {
    //--------------------------------------------------
    case EXEC:
        ecmd = (struct execcmd *)cmd;

        for (i = 0x00; ecmd->argv[i]; i++)
        {
            *ecmd->eargv[i] = 0x00;
        }
        break;
    //--------------------------------------------------
    case REDIR:
        rcmd = (struct redircmd *)cmd;

        nulterminate(rcmd->cmd);

        *rcmd->efile = 0x00;
        break;
    //--------------------------------------------------
    case PIPE:
        pcmd = (struct pipecmd *)cmd;

        nulterminate(pcmd->left);
        nulterminate(pcmd->right);
        break;
    //--------------------------------------------------
    case LIST:
        lcmd = (struct listcmd *)cmd;

        nulterminate(lcmd->left);
        nulterminate(lcmd->right);
        break;
    //--------------------------------------------------
    case BACK:
        bcmd = (struct backcmd *)cmd;

        nulterminate(bcmd->cmd);
        break;
    //--------------------------------------------------
    }

    return cmd;
}
