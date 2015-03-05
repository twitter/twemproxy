#!/bin/sh

port=6379
port=22121

debug="-v -d"
debug="-d"

timeout="-t 1"
timeout=""

# keys

printf '\ndel\n'
printf '*2\r\n$3\r\ndel\r\n$3\r\nfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*3\r\n$3\r\ndel\r\n$3\r\nfoo\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*3\r\n$3\r\nset\r\n$3\r\nfoo\r\n$3\r\noof\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*3\r\n$3\r\nset\r\n$3\r\nbar\r\n$3\r\nrab\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$3\r\ndel\r\n$3\r\nfoo\r\n$3\r\nbar\r\n$6\r\nfoobar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close

printf '\ndump\n'
printf '*2\r\n$4\r\ndump\r\n$3\r\nfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*3\r\n$3\r\nset\r\n$3\r\nfoo\r\n$3\r\noof\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*2\r\n$4\r\ndump\r\n$3\r\nfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close

printf '\nexists\n'
printf '*2\r\n$6\r\nexists\r\n$3\r\nfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*3\r\n$3\r\nset\r\n$3\r\nfoo\r\n$3\r\noof\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*2\r\n$6\r\nexists\r\n$3\r\nfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*2\r\n$3\r\ndel\r\n$3\r\nfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close

printf '\nexpire\n'
printf '*3\r\n$6\r\nexpire\r\n$3\r\nfoo\r\n$1\r\n0\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*3\r\n$3\r\nset\r\n$3\r\nfoo\r\n$3\r\noof\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*3\r\n$6\r\nexpire\r\n$3\r\nfoo\r\n$1\r\n0\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close

printf '\npersist\n'
printf '*2\r\n$7\r\npersist\r\n$3\r\nfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*3\r\n$3\r\nset\r\n$3\r\nfoo\r\n$3\r\noof\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*3\r\n$6\r\nexpire\r\n$3\r\nfoo\r\n$2\r\n10\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*2\r\n$7\r\npersist\r\n$3\r\nfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*2\r\n$7\r\npersist\r\n$3\r\nfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close

printf '\nexpireat\n'
printf '*3\r\n$8\r\nexpireat\r\n$3\r\nfoo\r\n$10\r\n1282463464\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*3\r\n$3\r\nset\r\n$3\r\nfoo\r\n$3\r\noof\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*3\r\n$8\r\nexpireat\r\n$3\r\nfoo\r\n$10\r\n1282463464\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close

printf '\nexpire\n'
printf '*3\r\n$7\r\npexpire\r\n$3\r\nfoo\r\n$1\r\n0\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*3\r\n$3\r\nset\r\n$3\r\nfoo\r\n$3\r\noof\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*3\r\n$7\r\npexpire\r\n$3\r\nfoo\r\n$1\r\n0\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close

printf '\nrestore\n'
printf '*2\r\n$3\r\ndel\r\n$3\r\nfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$7\r\nrestore\r\n$3\r\nfoo\r\n$1\r\n0\r\n$3\r\noof\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close

printf '\npttl\n'
printf '*2\r\n$4\r\npttl\r\n$3\r\nfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*3\r\n$3\r\nset\r\n$3\r\nfoo\r\n$3\r\noof\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*3\r\n$7\r\npexpire\r\n$3\r\nfoo\r\n$7\r\n1000000\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*2\r\n$4\r\npttl\r\n$3\r\nfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*2\r\n$3\r\ndel\r\n$3\r\nfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close

printf '\nttl\n'
printf '*2\r\n$4\r\npttl\r\n$3\r\nfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*2\r\n$3\r\nttl\r\n$3\r\nfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*3\r\n$3\r\nset\r\n$3\r\nfoo\r\n$3\r\noof\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*3\r\n$6\r\nexpire\r\n$3\r\nfoo\r\n$2\r\n10\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*2\r\n$3\r\nttl\r\n$3\r\nfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close

printf '\ntype\n'
printf '*2\r\n$4\r\ntype\r\n$3\r\nfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*2\r\n$4\r\ntype\r\n$3\r\noof\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close

# strings

printf '\nappend\n'
printf '*3\r\n$6\r\nappend\r\n$3\r\nfoo\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*3\r\n$6\r\nappend\r\n$3\r\n999\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close

printf '\nbitcount\n'
printf '*2\r\n$8\r\nbitcount\r\n$3\r\nfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$8\r\nbitcount\r\n$3\r\nfoo\r\n$1\r\n1\r\n$1\r\n1\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close

printf '\ndecr\n'
printf '*2\r\n$4\r\ndecr\r\n$7\r\ncounter\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close

printf '\ndecrby\n'
printf '*3\r\n$6\r\ndecrby\r\n$7\r\ncounter\r\n$3\r\n100\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close

printf '\nget\n'
printf '*2\r\n$3\r\nget\r\n$16\r\nnon-existent-key\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*2\r\n$3\r\nget\r\n$3\r\nfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*2\r\n$3\r\ndel\r\n$3\r\nfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*2\r\n$3\r\nget\r\n$3\r\nfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close

printf '\ngetbit\n'
printf '*2\r\n$3\r\ndel\r\n$3\r\nfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*3\r\n$6\r\ngetbit\r\n$3\r\nfoo\r\n$1\r\n1\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*3\r\n$3\r\nset\r\n$3\r\nfoo\r\n$3\r\noof\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*3\r\n$6\r\ngetbit\r\n$3\r\nfoo\r\n$1\r\n1\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close

printf '\ngetrange\n'
printf '*2\r\n$3\r\ndel\r\n$3\r\nfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$8\r\ngetrange\r\n$3\r\nfoo\r\n$1\r\n1\r\n$1\r\n2\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*3\r\n$3\r\nset\r\n$3\r\nfoo\r\n$3\r\noof\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$8\r\ngetrange\r\n$3\r\nfoo\r\n$1\r\n1\r\n$1\r\n2\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$8\r\ngetrange\r\n$3\r\nfoo\r\n$1\r\n1\r\n$3\r\n100\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close

printf '\ngetset\n'
printf '*2\r\n$3\r\ndel\r\n$3\r\nfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*3\r\n$6\r\ngetset\r\n$3\r\nfoo\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*2\r\n$3\r\nget\r\n$3\r\nfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close

printf '\nincr\n'
printf '*2\r\n$3\r\ndel\r\n$7\r\ncounter\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*2\r\n$4\r\nincr\r\n$7\r\ncounter\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close

printf '\nincrby\n'
printf '*3\r\n$6\r\nincrby\r\n$7\r\ncounter\r\n$3\r\n100\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close

printf '\nincrbyfloat\n'
printf '*3\r\n$11\r\nincrbyfloat\r\n$7\r\ncounter\r\n$5\r\n10.10\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close

printf '\nmget\n'
printf '*2\r\n$4\r\nmget\r\n$3\r\nfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*3\r\n$3\r\nset\r\n$3\r\nfoo\r\n$3\r\noof\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*2\r\n$4\r\nmget\r\n$3\r\nfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*3\r\n$4\r\nmget\r\n$3\r\nfoo\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*13\r\n$4\r\nmget\r\n$3\r\nfoo\r\n$3\r\nbar\r\n$3\r\nbar\r\n$3\r\nbar\r\n$3\r\nbar\r\n$3\r\nbar\r\n$3\r\nbar\r\n$3\r\nbar\r\n$3\r\nbar\r\n$3\r\nbar\r\n$3\r\nbar\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close

printf '\npsetex\n'
printf '*2\r\n$3\r\ndel\r\n$3\r\nfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$6\r\npsetex\r\n$3\r\nfoo\r\n$4\r\n1000\r\n$3\r\noof\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*2\r\n$3\r\nget\r\n$3\r\nfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close

printf '\nset\n'
printf '*3\r\n$3\r\nset\r\n$3\r\nfoo\r\n$3\r\noof\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close

printf '\nsetbit\n'
printf '*2\r\n$3\r\ndel\r\n$3\r\nfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$6\r\nsetbit\r\n$3\r\nfoo\r\n$1\r\n1\r\n$1\r\n1\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*2\r\n$3\r\nget\r\n$3\r\nfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*3\r\n$3\r\nset\r\n$3\r\nfoo\r\n$3\r\n000\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$6\r\nsetbit\r\n$3\r\nfoo\r\n$1\r\n1\r\n$1\r\n1\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*2\r\n$3\r\nget\r\n$3\r\nfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close

printf '\npsetex\n'
printf '*2\r\n$3\r\ndel\r\n$3\r\nfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$5\r\nsetex\r\n$3\r\nfoo\r\n$4\r\n1000\r\n$3\r\noof\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*2\r\n$3\r\nget\r\n$3\r\nfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close

printf '\nsetnx\n'
printf '*2\r\n$3\r\ndel\r\n$3\r\nfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*3\r\n$5\r\nsetnx\r\n$3\r\nfoo\r\n$3\r\noof\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*2\r\n$3\r\nget\r\n$3\r\nfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*3\r\n$3\r\nset\r\n$3\r\nfoo\r\n$3\r\noof\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*3\r\n$5\r\nsetnx\r\n$3\r\nfoo\r\n$3\r\nooo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*2\r\n$3\r\nget\r\n$3\r\nfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close

printf '\nsetrange\n'
printf '*2\r\n$3\r\ndel\r\n$3\r\nfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$8\r\nsetrange\r\n$3\r\nfoo\r\n$1\r\n1\r\n$3\r\noof\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*2\r\n$3\r\nget\r\n$3\r\nfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$8\r\nsetrange\r\n$3\r\nfoo\r\n$1\r\n4\r\n$3\r\noof\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*2\r\n$3\r\nget\r\n$3\r\nfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close

# hashes

printf '\nhdel\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\nhfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$4\r\nhdel\r\n$4\r\nhfoo\r\n$6\r\nfield1\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$4\r\nhset\r\n$4\r\nhfoo\r\n$6\r\nfield1\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$4\r\nhdel\r\n$4\r\nhfoo\r\n$6\r\nfield1\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close

printf '\nhexists\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\nhfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$4\r\nhdel\r\n$4\r\nhfoo\r\n$6\r\nfield1\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*3\r\n$7\r\nhexists\r\n$4\r\nhfoo\r\n$6\r\nfield1\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$4\r\nhset\r\n$4\r\nhfoo\r\n$6\r\nfield1\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*3\r\n$7\r\nhexists\r\n$4\r\nhfoo\r\n$6\r\nfield1\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close

printf '\nhget\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\nhfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$4\r\nhdel\r\n$4\r\nhfoo\r\n$6\r\nfield1\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*3\r\n$4\r\nhget\r\n$4\r\nhfoo\r\n$6\r\nfield1\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$4\r\nhset\r\n$4\r\nhfoo\r\n$6\r\nfield1\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*3\r\n$4\r\nhget\r\n$4\r\nhfoo\r\n$6\r\nfield1\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close

printf '\nhgetall\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\nhfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*2\r\n$7\r\nhgetall\r\n$4\r\nhfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$4\r\nhset\r\n$4\r\nhfoo\r\n$6\r\nfield1\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$4\r\nhset\r\n$4\r\nhfoo\r\n$6\r\n1dleif\r\n$3\r\nrab\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*2\r\n$7\r\nhgetall\r\n$4\r\nhfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close

printf '\nhincrby\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\nhfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$7\r\nhincrby\r\n$4\r\nhfoo\r\n$6\r\nfield1\r\n$3\r\n100\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*2\r\n$7\r\nhgetall\r\n$4\r\nhfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close

printf '\nhincrbyfloat\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\nhfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$12\r\nhincrbyfloat\r\n$4\r\nhfoo\r\n$6\r\nfield1\r\n$6\r\n100.12\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*2\r\n$7\r\nhgetall\r\n$4\r\nhfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close

printf '\nhkeys\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\nhfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*2\r\n$5\r\nhkeys\r\n$4\r\nhfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$4\r\nhset\r\n$4\r\nhfoo\r\n$6\r\n1dleif\r\n$3\r\nrab\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*2\r\n$5\r\nhkeys\r\n$4\r\nhfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$4\r\nhset\r\n$4\r\nhfoo\r\n$6\r\nfield1\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*2\r\n$5\r\nhkeys\r\n$4\r\nhfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close

printf '\nhlen\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\nhfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*2\r\n$4\r\nhlen\r\n$4\r\nhfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$4\r\nhset\r\n$4\r\nhfoo\r\n$6\r\n1dleif\r\n$3\r\nrab\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$4\r\nhset\r\n$4\r\nhfoo\r\n$6\r\nfield1\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*2\r\n$4\r\nhlen\r\n$4\r\nhfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close

printf '\nhmget\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\nhfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*3\r\n$5\r\nhmget\r\n$4\r\nhfoo\r\n$6\r\nfield1\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$4\r\nhset\r\n$4\r\nhfoo\r\n$6\r\nfield1\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$5\r\nhmget\r\n$4\r\nhfoo\r\n$6\r\nfield1\r\n$6\r\n1dleif\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$4\r\nhset\r\n$4\r\nhfoo\r\n$6\r\n1dleif\r\n$3\r\nrab\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$5\r\nhmget\r\n$4\r\nhfoo\r\n$6\r\nfield1\r\n$6\r\n1dleif\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close

printf '\nhmset\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\nhfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*6\r\n$5\r\nhmset\r\n$4\r\nhfoo\r\n$6\r\nfield1\r\n$3\r\nbar\r\n$6\r\nfield2\r\n$3\r\nbas\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*2\r\n$7\r\nhgetall\r\n$4\r\nhfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close

printf '\nhset\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\nhfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$4\r\nhset\r\n$4\r\nhfoo\r\n$6\r\nfield1\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*2\r\n$7\r\nhgetall\r\n$4\r\nhfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close

printf '\nhsetnx\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\nhfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$6\r\nhsetnx\r\n$4\r\nhfoo\r\n$6\r\nfield1\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*2\r\n$7\r\nhgetall\r\n$4\r\nhfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$6\r\nhsetnx\r\n$4\r\nhfoo\r\n$6\r\nfield1\r\n$3\r\nbas\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*2\r\n$7\r\nhgetall\r\n$4\r\nhfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close

printf '\nhvals\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\nhfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*2\r\n$5\r\nhvals\r\n$4\r\nhfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*6\r\n$5\r\nhmset\r\n$4\r\nhfoo\r\n$6\r\nfield1\r\n$3\r\nbar\r\n$6\r\nfield2\r\n$3\r\nbas\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*2\r\n$5\r\nhvals\r\n$4\r\nhfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close

# lists

printf '\nlindex\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\nlfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*3\r\n$6\r\nlindex\r\n$4\r\nlfoo\r\n$1\r\n1\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*3\r\n$5\r\nlpush\r\n$4\r\nlfoo\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*3\r\n$5\r\nlpush\r\n$4\r\nlfoo\r\n$3\r\nbas\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*3\r\n$6\r\nlindex\r\n$4\r\nlfoo\r\n$1\r\n1\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close

printf '\nlinsert\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\nlfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*3\r\n$5\r\nlpush\r\n$4\r\nlfoo\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*5\r\n$7\r\nlinsert\r\n$4\r\nlfoo\r\n$6\r\nBEFORE\r\n$3\r\nbar\r\n$3\r\nbaq\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*3\r\n$6\r\nlindex\r\n$4\r\nlfoo\r\n$1\r\n0\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close

printf '\nllen\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\nlfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*2\r\n$4\r\nllen\r\n$4\r\nlfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*3\r\n$5\r\nlpush\r\n$4\r\nlfoo\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*3\r\n$5\r\nlpush\r\n$4\r\nlfoo\r\n$3\r\nbas\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*2\r\n$4\r\nllen\r\n$4\r\nlfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close

printf '\nlpop\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\nlfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*2\r\n$4\r\nlpop\r\n$4\r\nlfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$5\r\nlpush\r\n$4\r\nlfoo\r\n$3\r\nbaq\r\n$3\r\nbap\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*2\r\n$4\r\nlpop\r\n$4\r\nlfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*2\r\n$4\r\nlpop\r\n$4\r\nlfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*2\r\n$4\r\nlpop\r\n$4\r\nlfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close

printf '\nlpush\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\nlfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*3\r\n$5\r\nlpush\r\n$4\r\nlfoo\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*3\r\n$5\r\nlpush\r\n$4\r\nlfoo\r\n$3\r\nbas\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$5\r\nlpush\r\n$4\r\nlfoo\r\n$3\r\nbaq\r\n$3\r\nbap\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$6\r\nlrange\r\n$4\r\nlfoo\r\n$1\r\n0\r\n$1\r\n3\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close

printf '\nlpushx\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\nlfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*3\r\n$6\r\nlpushx\r\n$4\r\nlfoo\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*3\r\n$5\r\nlpush\r\n$4\r\nlfoo\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*3\r\n$6\r\nlpushx\r\n$4\r\nlfoo\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*3\r\n$6\r\nlpushx\r\n$4\r\nlfoo\r\n$3\r\nbas\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$6\r\nlrange\r\n$4\r\nlfoo\r\n$1\r\n0\r\n$1\r\n3\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close

printf '\nlrange\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\nlfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$6\r\nlrange\r\n$4\r\nlfoo\r\n$1\r\n0\r\n$1\r\n2\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*6\r\n$5\r\nlpush\r\n$4\r\nlfoo\r\n$3\r\nbar\r\n$3\r\nbas\r\n$3\r\nbat\r\n$3\r\nbau\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$6\r\nlrange\r\n$4\r\nlfoo\r\n$1\r\n0\r\n$1\r\n2\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close

printf '\nlrem\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\nlfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$4\r\nlrem\r\n$4\r\nlfoo\r\n$1\r\n2\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*6\r\n$5\r\nlpush\r\n$4\r\nlfoo\r\n$3\r\nbar\r\n$3\r\nbar\r\n$3\r\nbar\r\n$3\r\nbau\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$4\r\nlrem\r\n$4\r\nlfoo\r\n$1\r\n2\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$4\r\nlrem\r\n$4\r\nlfoo\r\n$1\r\n2\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close

printf '\nlset\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\nlfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$4\r\nlset\r\n$4\r\nlfoo\r\n$1\r\n1\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*3\r\n$5\r\nlpush\r\n$4\r\nlfoo\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$4\r\nlset\r\n$4\r\nlfoo\r\n$1\r\n0\r\n$3\r\nbaq\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$4\r\nlset\r\n$4\r\nlfoo\r\n$1\r\n1\r\n$3\r\nbas\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$6\r\nlrange\r\n$4\r\nlfoo\r\n$1\r\n0\r\n$1\r\n2\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close

printf '\nltrim\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\nlfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$5\r\nltrim\r\n$4\r\nlfoo\r\n$1\r\n0\r\n$1\r\n2\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*5\r\n$5\r\nlpush\r\n$4\r\nlfoo\r\n$3\r\nbaq\r\n$3\r\nbap\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close

printf '\nrpop\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\nlfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*2\r\n$4\r\nrpop\r\n$4\r\nlfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$5\r\nlpush\r\n$4\r\nlfoo\r\n$3\r\nbaq\r\n$3\r\nbap\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*2\r\n$4\r\nrpop\r\n$4\r\nlfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*2\r\n$4\r\nrpop\r\n$4\r\nlfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*2\r\n$4\r\nrpop\r\n$4\r\nlfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close

printf '\nrpoplpush\n'
printf '*2\r\n$3\r\ndel\r\n$6\r\n{lfoo}\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*2\r\n$3\r\ndel\r\n$7\r\n{lfoo}2\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*5\r\n$5\r\nlpush\r\n$6\r\n{lfoo}\r\n$3\r\nbar\r\n$3\r\nbaq\r\n$3\r\nbap\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*3\r\n$9\r\nrpoplpush\r\n$6\r\n{lfoo}\r\n$7\r\n{lfoo}2\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*3\r\n$9\r\nrpoplpush\r\n$6\r\n{lfoo}\r\n$7\r\n{lfoo}2\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$6\r\nlrange\r\n$6\r\n{lfoo}\r\n$1\r\n0\r\n$1\r\n3\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$6\r\nlrange\r\n$7\r\n{lfoo}2\r\n$1\r\n0\r\n$1\r\n3\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close

printf '\nrpush\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\nlfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*3\r\n$5\r\nrpush\r\n$4\r\nlfoo\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*3\r\n$5\r\nrpush\r\n$4\r\nlfoo\r\n$3\r\nbas\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$5\r\nrpush\r\n$4\r\nlfoo\r\n$3\r\nbat\r\n$3\r\nbau\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$6\r\nlrange\r\n$4\r\nlfoo\r\n$1\r\n0\r\n$1\r\n3\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close

printf '\nrpushx\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\nlfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*3\r\n$6\r\nrpushx\r\n$4\r\nlfoo\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*3\r\n$5\r\nrpush\r\n$4\r\nlfoo\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*3\r\n$6\r\nrpushx\r\n$4\r\nlfoo\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*3\r\n$6\r\nrpushx\r\n$4\r\nlfoo\r\n$3\r\nbas\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$6\r\nlrange\r\n$4\r\nlfoo\r\n$1\r\n0\r\n$1\r\n3\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close

# sets

printf '\nsadd\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\nsfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$4\r\nsadd\r\n$4\r\nsfoo\r\n$3\r\nbar\r\n$3\r\nbas\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*2\r\n$8\r\nsmembers\r\n$4\r\nsfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close

printf '\nscard\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\nsfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*2\r\n$5\r\nscard\r\n$4\r\nsfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$4\r\nsadd\r\n$4\r\nsfoo\r\n$3\r\nbar\r\n$3\r\nbas\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*2\r\n$5\r\nscard\r\n$4\r\nsfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close

printf '\nsdiff\n'
printf '*2\r\n$3\r\ndel\r\n$6\r\n{sfoo}\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*2\r\n$3\r\ndel\r\n$7\r\n{sfoo}2\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$4\r\nsadd\r\n$6\r\n{sfoo}\r\n$3\r\nbar\r\n$3\r\nbas\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*3\r\n$4\r\nsadd\r\n$7\r\n{sfoo}2\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*3\r\n$5\r\nsdiff\r\n$6\r\n{sfoo}\r\n$7\r\n{sfoo}2\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close

printf '\nsdiffstore\n'
printf '*2\r\n$3\r\ndel\r\n$6\r\n{sfoo}\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*2\r\n$3\r\ndel\r\n$7\r\n{sfoo}2\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*2\r\n$3\r\ndel\r\n$7\r\n{sfoo}3\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$4\r\nsadd\r\n$6\r\n{sfoo}\r\n$3\r\nbar\r\n$3\r\nbas\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*3\r\n$4\r\nsadd\r\n$7\r\n{sfoo}2\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$10\r\nsdiffstore\r\n$7\r\n{sfoo}3\r\n$6\r\n{sfoo}\r\n$7\r\n{sfoo}2\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*2\r\n$8\r\nsmembers\r\n$7\r\n{sfoo}3\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close

printf '\nsinter\n'
printf '*2\r\n$3\r\ndel\r\n$6\r\n{sfoo}\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*2\r\n$3\r\ndel\r\n$7\r\n{sfoo}2\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$4\r\nsadd\r\n$6\r\n{sfoo}\r\n$3\r\nbar\r\n$3\r\nbas\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*3\r\n$4\r\nsadd\r\n$7\r\n{sfoo}2\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*3\r\n$6\r\nsinter\r\n$6\r\n{sfoo}\r\n$7\r\n{sfoo}2\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close

printf '\nsinterstore\n'
printf '*2\r\n$3\r\ndel\r\n$6\r\n{sfoo}\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*2\r\n$3\r\ndel\r\n$7\r\n{sfoo}2\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*2\r\n$3\r\ndel\r\n$7\r\n{sfoo}3\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$4\r\nsadd\r\n$6\r\n{sfoo}\r\n$3\r\nbar\r\n$3\r\nbas\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*3\r\n$4\r\nsadd\r\n$7\r\n{sfoo}2\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$11\r\nsinterstore\r\n$7\r\n{sfoo}3\r\n$6\r\n{sfoo}\r\n$7\r\n{sfoo}2\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*2\r\n$8\r\nsmembers\r\n$7\r\n{sfoo}3\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close

printf '\nsismember\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\nsfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*3\r\n$9\r\nsismember\r\n$4\r\nsfoo\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$4\r\nsadd\r\n$4\r\nsfoo\r\n$3\r\nbar\r\n$3\r\nbas\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*3\r\n$9\r\nsismember\r\n$4\r\nsfoo\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*3\r\n$9\r\nsismember\r\n$4\r\nsfoo\r\n$3\r\nrab\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close

printf '\nsmembers\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\nsfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*2\r\n$8\r\nsmembers\r\n$4\r\nsfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$4\r\nsadd\r\n$4\r\nsfoo\r\n$3\r\nbar\r\n$3\r\nbas\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*2\r\n$8\r\nsmembers\r\n$4\r\nsfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close

printf '\nsmove\n'
printf '*2\r\n$3\r\ndel\r\n$6\r\n{sfoo}\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*2\r\n$3\r\ndel\r\n$7\r\n{sfoo}2\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$4\r\nsadd\r\n$6\r\n{sfoo}\r\n$3\r\nbar\r\n$3\r\nbas\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$5\r\nsmove\r\n$6\r\n{sfoo}\r\n$7\r\n{sfoo}2\r\n$3\r\nbas\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*2\r\n$8\r\nsmembers\r\n$6\r\n{sfoo}\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*2\r\n$8\r\nsmembers\r\n$7\r\n{sfoo}2\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close

printf '\nspop\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\nsfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*2\r\n$4\r\nspop\r\n$4\r\nsfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$4\r\nsadd\r\n$4\r\nsfoo\r\n$3\r\nbar\r\n$3\r\nbas\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*2\r\n$4\r\nspop\r\n$4\r\nsfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close

printf '\nsrandmember\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\nsfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*2\r\n$11\r\nsrandmember\r\n$4\r\nsfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$4\r\nsadd\r\n$4\r\nsfoo\r\n$3\r\nbar\r\n$3\r\nbas\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*2\r\n$11\r\nsrandmember\r\n$4\r\nsfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*2\r\n$11\r\nsrandmember\r\n$4\r\nsfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*2\r\n$11\r\nsrandmember\r\n$4\r\nsfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*3\r\n$11\r\nsrandmember\r\n$4\r\nsfoo\r\n$1\r\n2\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close

printf '\nsrem\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\nsfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*3\r\n$4\r\nsrem\r\n$4\r\nsfoo\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*5\r\n$4\r\nsadd\r\n$4\r\nsfoo\r\n$3\r\nbar\r\n$3\r\nbas\r\n$3\r\nbat\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*3\r\n$4\r\nsrem\r\n$4\r\nsfoo\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*5\r\n$4\r\nsrem\r\n$4\r\nsfoo\r\n$3\r\nbas\r\n$3\r\nbat\r\n$3\r\nrab\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close

printf '\nsunion\n'
printf '*2\r\n$3\r\ndel\r\n$6\r\n{sfoo}\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*2\r\n$3\r\ndel\r\n$7\r\n{sfoo}2\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$4\r\nsadd\r\n$6\r\n{sfoo}\r\n$3\r\nbar\r\n$3\r\nbas\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*3\r\n$4\r\nsadd\r\n$7\r\n{sfoo}2\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*3\r\n$6\r\nsunion\r\n$6\r\n{sfoo}\r\n$7\r\n{sfoo}2\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close

printf '\nsunionstore\n'
printf '*2\r\n$3\r\ndel\r\n$6\r\n{sfoo}\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*2\r\n$3\r\ndel\r\n$7\r\n{sfoo}2\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*2\r\n$3\r\ndel\r\n$7\r\n{sfoo}3\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$4\r\nsadd\r\n$6\r\n{sfoo}\r\n$3\r\nbar\r\n$3\r\nbas\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*3\r\n$4\r\nsadd\r\n$7\r\n{sfoo}2\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$11\r\nsunionstore\r\n$7\r\n{sfoo}3\r\n$6\r\n{sfoo}\r\n$7\r\n{sfoo}2\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*2\r\n$8\r\nsmembers\r\n$7\r\n{sfoo}3\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close

# sorted sets

printf '\nzadd\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\nzfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$4\r\nzadd\r\n$4\r\nzfoo\r\n$3\r\n100\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$4\r\nzadd\r\n$4\r\nzfoo\r\n$3\r\n100\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*6\r\n$4\r\nzadd\r\n$4\r\nzfoo\r\n$3\r\n101\r\n$3\r\nbat\r\n$3\r\n102\r\n$3\r\nbau\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close

printf '\nzcard\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\nzfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*2\r\n$5\r\nzcard\r\n$4\r\nzfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*8\r\n$4\r\nzadd\r\n$4\r\nzfoo\r\n$3\r\n100\r\n$3\r\nbar\r\n$3\r\n101\r\n$3\r\nbat\r\n$3\r\n102\r\n$3\r\nbau\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*2\r\n$5\r\nzcard\r\n$4\r\nzfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close

printf '\nzcount\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\nzfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$6\r\nzcount\r\n$4\r\nzfoo\r\n$3\r\n100\r\n$3\r\n101\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*8\r\n$4\r\nzadd\r\n$4\r\nzfoo\r\n$3\r\n100\r\n$3\r\nbar\r\n$3\r\n101\r\n$3\r\nbat\r\n$3\r\n102\r\n$3\r\nbau\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$6\r\nzcount\r\n$4\r\nzfoo\r\n$3\r\n100\r\n$3\r\n101\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$6\r\nzcount\r\n$4\r\nzfoo\r\n$4\r\n-inf\r\n$4\r\n+inf\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close

printf '\nzincrby\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\nzfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$7\r\nzincrby\r\n$4\r\nzfoo\r\n$3\r\n100\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$7\r\nzincrby\r\n$4\r\nzfoo\r\n$3\r\n100\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close

printf '\nzinterstore\n'
printf '*2\r\n$3\r\ndel\r\n$6\r\n{zfoo}\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*2\r\n$3\r\ndel\r\n$7\r\n{zfoo}2\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*2\r\n$3\r\ndel\r\n$7\r\n{zfoo}3\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*8\r\n$4\r\nzadd\r\n$6\r\n{zfoo}\r\n$3\r\n100\r\n$3\r\nbar\r\n$3\r\n101\r\n$3\r\nbat\r\n$3\r\n102\r\n$3\r\nbau\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*6\r\n$4\r\nzadd\r\n$7\r\n{zfoo}2\r\n$3\r\n100\r\n$3\r\nbar\r\n$3\r\n101\r\n$3\r\nbat\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*5\r\n$11\r\nzinterstore\r\n$7\r\n{zfoo}3\r\n$1\r\n2\r\n$6\r\n{zfoo}\r\n$7\r\n{zfoo}2\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*5\r\n$6\r\nzrange\r\n$7\r\n{zfoo}3\r\n$1\r\n0\r\n$1\r\n3\r\n$10\r\nWITHSCORES\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close

printf '\nzrange\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\nzfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$6\r\nzrange\r\n$4\r\nzfoo\r\n$1\r\n0\r\n$1\r\n3\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*8\r\n$4\r\nzadd\r\n$4\r\nzfoo\r\n$3\r\n100\r\n$3\r\nbar\r\n$3\r\n101\r\n$3\r\nbat\r\n$3\r\n102\r\n$3\r\nbau\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$6\r\nzrange\r\n$4\r\nzfoo\r\n$1\r\n0\r\n$1\r\n3\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*5\r\n$6\r\nzrange\r\n$4\r\nzfoo\r\n$1\r\n0\r\n$1\r\n3\r\n$10\r\nWITHSCORES\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close

printf '\nzrangebyscore\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\nzfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$13\r\nzrangebyscore\r\n$4\r\nzfoo\r\n$3\r\n100\r\n$3\r\n101\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*8\r\n$4\r\nzadd\r\n$4\r\nzfoo\r\n$3\r\n100\r\n$3\r\nbar\r\n$3\r\n101\r\n$3\r\nbat\r\n$3\r\n102\r\n$3\r\nbau\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$13\r\nzrangebyscore\r\n$4\r\nzfoo\r\n$3\r\n100\r\n$3\r\n101\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close

printf '\nzrank\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\nzfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*3\r\n$5\r\nzrank\r\n$4\r\nzfoo\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*8\r\n$4\r\nzadd\r\n$4\r\nzfoo\r\n$3\r\n100\r\n$3\r\nbar\r\n$3\r\n101\r\n$3\r\nbat\r\n$3\r\n102\r\n$3\r\nbau\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*3\r\n$5\r\nzrank\r\n$4\r\nzfoo\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close

printf '\nzrem\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\nzfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*3\r\n$4\r\nzrem\r\n$4\r\nzfoo\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*8\r\n$4\r\nzadd\r\n$4\r\nzfoo\r\n$3\r\n100\r\n$3\r\nbar\r\n$3\r\n101\r\n$3\r\nbat\r\n$3\r\n102\r\n$3\r\nbau\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*8\r\n$4\r\nzrem\r\n$4\r\nzfoo\r\n$3\r\n100\r\n$3\r\nbar\r\n$3\r\n101\r\n$3\r\nbat\r\n$3\r\n102\r\n$3\r\nbau\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close

printf '\nzremrangebyrank\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\nzfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$15\r\nzremrangebyrank\r\n$4\r\nzfoo\r\n$1\r\n0\r\n$1\r\n1\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*8\r\n$4\r\nzadd\r\n$4\r\nzfoo\r\n$3\r\n100\r\n$3\r\nbar\r\n$3\r\n101\r\n$3\r\nbat\r\n$3\r\n102\r\n$3\r\nbau\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$15\r\nzremrangebyrank\r\n$4\r\nzfoo\r\n$1\r\n0\r\n$1\r\n1\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close

printf '\nzremrangebyscore\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\nzfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$16\r\nzremrangebyscore\r\n$4\r\nzfoo\r\n$3\r\n100\r\n$3\r\n101\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*8\r\n$4\r\nzadd\r\n$4\r\nzfoo\r\n$3\r\n100\r\n$3\r\nbar\r\n$3\r\n101\r\n$3\r\nbat\r\n$3\r\n102\r\n$3\r\nbau\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$16\r\nzremrangebyscore\r\n$4\r\nzfoo\r\n$3\r\n100\r\n$3\r\n101\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close

printf '\nzrevrange\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\nzfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$9\r\nzrevrange\r\n$4\r\nzfoo\r\n$1\r\n0\r\n$1\r\n2\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*8\r\n$4\r\nzadd\r\n$4\r\nzfoo\r\n$3\r\n100\r\n$3\r\nbar\r\n$3\r\n101\r\n$3\r\nbat\r\n$3\r\n102\r\n$3\r\nbau\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$9\r\nzrevrange\r\n$4\r\nzfoo\r\n$1\r\n0\r\n$1\r\n2\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close

printf '\nzrevrangebyscore\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\nzfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$16\r\nzrevrangebyscore\r\n$4\r\nzfoo\r\n$3\r\n101\r\n$3\r\n100\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*8\r\n$4\r\nzadd\r\n$4\r\nzfoo\r\n$3\r\n100\r\n$3\r\nbar\r\n$3\r\n101\r\n$3\r\nbat\r\n$3\r\n102\r\n$3\r\nbau\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$16\r\nzrevrangebyscore\r\n$4\r\nzfoo\r\n$3\r\n101\r\n$3\r\n100\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close

printf '\nzrevrank\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\nzfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*3\r\n$8\r\nzrevrank\r\n$4\r\nzfoo\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*8\r\n$4\r\nzadd\r\n$4\r\nzfoo\r\n$3\r\n100\r\n$3\r\nbar\r\n$3\r\n101\r\n$3\r\nbat\r\n$3\r\n102\r\n$3\r\nbau\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*3\r\n$8\r\nzrevrank\r\n$4\r\nzfoo\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close

printf '\nzscore\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\nzfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*3\r\n$6\r\nzscore\r\n$4\r\nzfoo\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*8\r\n$4\r\nzadd\r\n$4\r\nzfoo\r\n$3\r\n100\r\n$3\r\nbar\r\n$3\r\n101\r\n$3\r\nbat\r\n$3\r\n102\r\n$3\r\nbau\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*3\r\n$6\r\nzscore\r\n$4\r\nzfoo\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close

printf '\nzunionstore\n'
printf '*2\r\n$3\r\ndel\r\n$6\r\n{zfoo}\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*2\r\n$3\r\ndel\r\n$7\r\n{zfoo}2\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*2\r\n$3\r\ndel\r\n$7\r\n{zfoo}3\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*8\r\n$4\r\nzadd\r\n$6\r\n{zfoo}\r\n$3\r\n100\r\n$3\r\nbar\r\n$3\r\n101\r\n$3\r\nbat\r\n$3\r\n102\r\n$3\r\nbau\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*6\r\n$4\r\nzadd\r\n$7\r\n{zfoo}2\r\n$3\r\n100\r\n$3\r\nbar\r\n$3\r\n101\r\n$3\r\nbat\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*5\r\n$11\r\nzunionstore\r\n$7\r\n{zfoo}3\r\n$1\r\n2\r\n$6\r\n{zfoo}\r\n$7\r\n{zfoo}2\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*5\r\n$6\r\nzrange\r\n$7\r\n{zfoo}3\r\n$1\r\n0\r\n$1\r\n3\r\n$10\r\nWITHSCORES\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close

printf '\nzlexcount\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\nzfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$9\r\nzlexcount\r\n$4\r\nzfoo\r\n$2\r\n(a\r\n$2\r\n(z\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*8\r\n$4\r\nzadd\r\n$4\r\nzfoo\r\n$3\r\n100\r\n$3\r\nbar\r\n$3\r\n101\r\n$3\r\nbat\r\n$3\r\n102\r\n$3\r\nbau\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$9\r\nzlexcount\r\n$4\r\nzfoo\r\n$2\r\n(a\r\n$4\r\n[bat\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$9\r\nzlexcount\r\n$4\r\nzfoo\r\n$1\r\n-\r\n$1\r\n+\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close

printf '\nzrangebylex\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\nzfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$11\r\nzrangebylex\r\n$4\r\nzfoo\r\n$2\r\n(a\r\n$2\r\n(z\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*8\r\n$4\r\nzadd\r\n$4\r\nzfoo\r\n$3\r\n100\r\n$3\r\nbar\r\n$3\r\n101\r\n$3\r\nbat\r\n$3\r\n102\r\n$3\r\nbau\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$11\r\nzrangebylex\r\n$4\r\nzfoo\r\n$2\r\n(a\r\n$4\r\n[bat\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$11\r\nzrangebylex\r\n$4\r\nzfoo\r\n$1\r\n-\r\n$1\r\n+\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close

printf '\nzremrangebylex\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\nzfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$14\r\nzremrangebylex\r\n$4\r\nzfoo\r\n$2\r\n(a\r\n$2\r\n(z\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*8\r\n$4\r\nzadd\r\n$4\r\nzfoo\r\n$3\r\n100\r\n$3\r\nbar\r\n$3\r\n101\r\n$3\r\nbat\r\n$3\r\n102\r\n$3\r\nbau\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$14\r\nzremrangebylex\r\n$4\r\nzfoo\r\n$2\r\n(a\r\n$2\r\n(z\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$14\r\nzremrangebylex\r\n$4\r\nzfoo\r\n$1\r\n-\r\n$1\r\n+\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close

# hyperloglog

printf '\npfadd\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\npfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$5\r\npfadd\r\n$4\r\npfoo\r\n$3\r\nbar\r\n$3\r\nbas\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*2\r\n$7\r\npfcount\r\n$4\r\npfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close

printf '\npfcount\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\npfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*2\r\n$7\r\npfcount\r\n$4\r\npfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$5\r\npfadd\r\n$4\r\npfoo\r\n$3\r\nbar\r\n$3\r\nbas\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*2\r\n$7\r\npfcount\r\n$4\r\npfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close

printf '\npfmerge\n'
printf '*2\r\n$3\r\ndel\r\n$6\r\n{pfoo}\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*2\r\n$3\r\ndel\r\n$7\r\n{pfoo}2\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*2\r\n$3\r\ndel\r\n$7\r\n{pfoo}3\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$5\r\npfadd\r\n$6\r\n{pfoo}\r\n$4\r\nsfoo\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*3\r\n$5\r\npfadd\r\n$7\r\n{pfoo}2\r\n$3\r\nbas\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*5\r\n$7\r\npfmerge\r\n$7\r\n{pfoo}3\r\n$1\r\n2\r\n$6\r\n{pfoo}\r\n$7\r\n{pfoo}2\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*2\r\n$7\r\npfcount\r\n$7\r\n{pfoo}3\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close

# scripting

printf '\neval\n'
printf '*2\r\n$4\r\neval\r\n$10\r\nreturn 123\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*3\r\n$4\r\neval\r\n$10\r\nreturn 123\r\n$1\r\n0\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*3\r\n$4\r\neval\r\n$10\r\nreturn 123\r\n$1\r\n1\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$4\r\neval\r\n$10\r\nreturn 123\r\n$1\r\n1\r\n$1\r\n1\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*7\r\n$4\r\neval\r\n$40\r\nreturn {KEYS[1],KEYS[2],ARGV[1],ARGV[2]}\r\n$1\r\n2\r\n$9\r\nkey1{tag}\r\n$4\r\narg1\r\n$9\r\nkey2{tag}\r\n$4\r\narg2\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*9\r\n$4\r\neval\r\n$56\r\nreturn {KEYS[1],KEYS[2],KEYS[3],ARGV[1],ARGV[2],ARGV[3]}\r\n$1\r\n3\r\n$9\r\nkey1{tag}\r\n$4\r\narg1\r\n$9\r\nkey2{tag}\r\n$4\r\narg2\r\n$9\r\nkey3{tag}\r\n$4\r\narg3\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
printf '*4\r\n$4\r\neval\r\n$11\r\nreturn {10}\r\n$1\r\n1\r\n$4\r\nTEMP\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close

# quit
printf '\nquit\n'
printf '*1\r\n$4\r\nquit\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port},shut-close
