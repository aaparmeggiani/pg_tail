## pg_tail

Watches last lines of a PostgreSQL table like in a "tail -f".
Inspired by [mysqltail](http://www.vanheusden.com/mysqltail/).

## Install
PostgreSQL is needed and make sure yours have pg_config and libpq.
```console
git clone https://github.com/aaparmeggiani/pg_tail.git
cd pg_tail
make
make install
```
## Usage
```console
% pg_tail [OPTIONS] -t TABLE -c uKEY[,COL1,..,COLn]

Options:
    -d, --dbname=DBNAME         database to connect to
    -h, --host=HOSTNAME         database server host or socket directory
    -p, --port=PORT             database server port number
    -U, --username=NAME         connect as specified database user

    -t, --table=TABLE           table to watch
    -c, --columns=COL1..COLn    columns to watch, the first one must be an ordered primary key (sequence)
    -i, --interval=SECONDS      database polling interval in seconds (default: 10)
    -s, --separator=CHAR        sets a column delimiter (an no column alignment)
    -n  NUM                     number of lines in the first polling (default: 5)
    -v, --version               version info
```

## Example

```console
% pg_tail -d database -t users -c id,login,email
id | login  | email              |
1  | system | system@example.com |
2  | global | global@example.com |
3  | teller | teller@example.com |
```

## Dockerized
```
  docker run -it --init --rm aaparmeggiani/pg_tail -h host -d database -U user -t table -c key
```
(_don't forget **--init** or you'll find yourself  trapped without ctrl-c_  =)

## License
[The MIT License (MIT)](http://www.opensource.org/licenses/MIT)

