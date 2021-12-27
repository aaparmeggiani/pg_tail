
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>
#include <libpq-fe.h>

#define VERSION   "0.8"
#define INTERVAL  10         /* default polling interval in seconds*/
#define LINES     5          /* default number of lines in the first poll */
#define SEPARATOR " | "      /* default column delimiter */

#define MAX(n1, n2)   ((n1) > (n2) ? (n1) : (n2))

static void help(void) {
  printf("\npg_tail - watches the last records of a PostgreSQL table.\n\n");
  printf("Usage:\n");
  printf("  pg_tail [OPTIONS] -t TABLE -c uKEY[,COL1,..,COLn]\n");

  printf("\nOptions:\n");
  printf("  -d, --dbname=DBNAME           database to connect to\n");
  printf("  -h, --host=HOSTNAME           database server host or socket directory\n");
  printf("  -p, --port=PORT               database server port number\n");
  printf("  -U, --username=NAME           connect as specified database user\n");
  printf("  -W, --password                prompts for the user password\n\n");

  printf("  -t, --table=TABLE             table to watch\n");
  printf("  -c, --columns=COL1..COLn      columns to watch, the first one must be an ordered primary key (sequence)\n");
  printf("  -i, --interval=SECONDS        database polling interval in seconds (default: %d)\n", INTERVAL);
  printf("  -s, --separator=CHAR          sets a column delimiter (an no column alignment) \n");
  printf("  -n  NUM                       number of lines in the first poll (default: %d)\n", LINES);
  printf("  -j, --json                    output as json\n");
  printf("  -v, --version                 version info\n\n");
}

static void exit_nicely(PGconn *conn) {
  if(conn) { PQfinish(conn); }
  exit(1);
}

int main(int argc, char **argv)
{
  PGconn      *conn;
  PGresult    *res;
  const char  *op_dbname      = getenv("PGDATABASE");
  const char  *op_pghost      = getenv("PGHOST");
  const char  *op_pgport      = getenv("PGPORT");
  const char  *op_username    = getenv("PGUSER");
  const char  *op_table       = getenv("PGTAILTABLE");
  const char  *op_key         = getenv("PGTAILKEY");
  const char  *op_columns     = getenv("PGTAILCOLUMNS");
  const char  *op_separator   = getenv("PGTAILSEPARATOR") ? getenv("PGTAILSEPARATOR") : SEPARATOR;
  int         op_interval     = getenv("PGTAILINTERVAL") ? atoi(getenv("PGTAILINTERVAL")) : INTERVAL;
  int         op_n            = getenv("PGTAILLINES") ? atoi(getenv("PGTAILLINES")) : LINES;
  int         op_align        = getenv("PGTAILALIGN") ? atoi(getenv("PGTAILALIGN")) : 1;
  int         op_json         = getenv("PGTAILJSON") ? atoi(getenv("PGTAILJSON")) : 0;

  char        *current_key    = NULL;
  char        query[2000]     = {};
  char        *password       = getenv("PGPASSWORD");
  int         col_length[500] = {};
  int         num_rows        = 0;
  int         num_fields      = 0;
  int         i,j,c;
  int         optindex;

  static struct option long_options[] = {
    {"dbname",    required_argument, NULL, 'd'},
    {"host",      required_argument, NULL, 'h'},
    {"port",      required_argument, NULL, 'p'},
    {"username",  required_argument, NULL, 'U'},
    {"password",  no_argument,       NULL, 'W'},
    {"table",     required_argument, NULL, 't'},
    {"columns",   required_argument, NULL, 'c'},
    {"separator", required_argument, NULL, 's'},
    {"interval",  required_argument, NULL, 'i'},
    {"json",      no_argument,       NULL, 'j'},
    {"version",   no_argument,       NULL, 'v'},
    {NULL, 0, NULL, 0}
  };

  if ( argc <= 1 ) {
    help();
    exit_nicely(0);
  }
  else if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-?") == 0) {
    help();
    exit_nicely(0);
  }

  while ((c = getopt_long(argc, argv, "d:h:p:t:c:i:s:n:U:Wjv", long_options, &optindex)) != -1) {

    switch (c) {

      case 'd':
        op_dbname = strdup(optarg);
        break;

      case 'h':
        op_pghost = strdup(optarg);
        break;

      case 'p':
        op_pgport = strdup(optarg);
        break;

      case 't':
        op_table = strdup(optarg);
        break;

      case 'c':
        op_columns = strdup(optarg);
        op_key = strtok(strdup(op_columns), ",");
        break;

      case 'i':
        op_interval = atoi(optarg);
        break;

      case 's':
        op_separator = strdup(optarg);
        op_align = 0;
        break;

      case 'n':
        op_n = MAX(1, atoi(optarg));
        break;

      case 'U':
        op_username = strdup(optarg);
        break;
      
      case 'W':
        password = getpass("Password: ");
        break;

      case 'j':
        op_json = 1;
        break;

      case 'v':
        printf("%s\n", VERSION);
        exit_nicely(0);
        break;

      default:
        fprintf(stderr, "Try pg_tail --help for more information.\n");
        exit_nicely(0);
    }
  }

  if(!op_table || !op_columns) {
    fprintf(stderr, "Missing table or key (column).\n");
    exit_nicely(0);
  }
  conn = PQsetdbLogin(op_pghost, op_pgport, NULL, NULL, op_dbname, op_username, password);
  if(password){ memset_s(password, strlen(password), 0,  strlen(password)); }

  if (PQstatus(conn) != CONNECTION_OK) {
    fprintf(stderr, "Connection to database failed.\n%s\n", PQerrorMessage(conn));
    exit_nicely(conn);
  }

  while(1) {

    if(current_key)
      snprintf(query, sizeof(query),
        "SELECT %s,row_to_json(row(%s)) FROM %s WHERE %s > '%s' ORDER BY %s ASC",
        op_columns, op_columns, op_table, op_key, current_key, op_key);
    else
      snprintf(query, sizeof(query),
       "SELECT * FROM (SELECT %s,row_to_json(row(%s)) FROM %s ORDER BY %s DESC LIMIT %d) AS tmp ORDER BY %s ASC",
        op_columns, op_columns, op_table, op_key, op_n, op_key);

    res = PQexec(conn, query);
    if(PQresultStatus(res) != PGRES_TUPLES_OK) {
      fprintf(stderr, "%s", PQerrorMessage(conn));
      PQclear(res);
      exit_nicely(conn);
    }

    num_rows  = PQntuples(res);
    num_fields = PQnfields(res);

    /* columns sizes */
    for (i = 0; i < num_rows; i++)
      for (j = 0; j < num_fields; j++) {
        col_length[j] = MAX( col_length[j], strlen(PQfname(res, j)) );
        col_length[j] = MAX( col_length[j], strlen(PQgetvalue(res, i, j)) );
      }

    /* header if 1st lap */
    if(!current_key && num_rows > 0 && !op_json) {
      for(i = 0; i < num_fields-1; i++)
        printf("%-*s%s", col_length[i], PQfname(res, i), op_separator);
      printf("\n");
      fflush(stdout);
    }

    /* rows */
    for (i = 0; i < num_rows; i++) {
      if(op_json){
        printf("%s", PQgetvalue(res, i, num_fields-1));
      }
      else{
        for (j = 0; j < num_fields-1; j++)
          printf("%-*s%s", (op_align * col_length[j]), PQgetvalue(res, i, j), op_separator);
      }
      printf("\n");
      fflush(stdout);
    }

    if(num_rows > 0) {
      free(current_key);
      current_key = strdup(PQgetvalue(res, i-1, 0));
    }

    PQclear(res);
    sleep(op_interval);
  }

}
