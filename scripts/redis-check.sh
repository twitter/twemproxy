#!/bin/sh

port=7379

#debug="-v -d"
#debug="-d"

timeout="-t 1"
#timeout=""

# ping

printf '\nping\n'
printf '*2\r\n$4\r\nping\r\n$11\r\nhello world\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}


# keys

printf '\ndel\n'
printf '*2\r\n$3\r\ndel\r\n$3\r\nfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*3\r\n$3\r\ndel\r\n$3\r\nfoo\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*3\r\n$3\r\nset\r\n$3\r\nfoo\r\n$3\r\noof\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*3\r\n$3\r\nset\r\n$3\r\nbar\r\n$3\r\nrab\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$3\r\ndel\r\n$3\r\nfoo\r\n$3\r\nbar\r\n$6\r\nfoobar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$3\r\ndel\r\n$3\r\nfoo\r\n$3\r\nbar\r\n$6\r\nfoobar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}

printf '\ndump\n'
printf '*2\r\n$4\r\ndump\r\n$3\r\nfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*3\r\n$3\r\nset\r\n$3\r\nfoo\r\n$3\r\noof\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*2\r\n$4\r\ndump\r\n$3\r\nfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}

printf '\nexists\n'
printf '*2\r\n$6\r\nexists\r\n$3\r\nfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*3\r\n$3\r\nset\r\n$3\r\nfoo\r\n$3\r\noof\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*2\r\n$6\r\nexists\r\n$3\r\nfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*2\r\n$3\r\ndel\r\n$3\r\nfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}

printf '\nexpire\n'
printf '*3\r\n$6\r\nexpire\r\n$3\r\nfoo\r\n$1\r\n0\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*3\r\n$3\r\nset\r\n$3\r\nfoo\r\n$3\r\noof\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*3\r\n$6\r\nexpire\r\n$3\r\nfoo\r\n$1\r\n0\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}

printf '\npersist\n'
printf '*2\r\n$7\r\npersist\r\n$3\r\nfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*3\r\n$3\r\nset\r\n$3\r\nfoo\r\n$3\r\noof\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*3\r\n$6\r\nexpire\r\n$3\r\nfoo\r\n$2\r\n10\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*2\r\n$7\r\npersist\r\n$3\r\nfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*2\r\n$7\r\npersist\r\n$3\r\nfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}

printf '\nexpireat\n'
printf '*3\r\n$8\r\nexpireat\r\n$3\r\nfoo\r\n$10\r\n1282463464\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*3\r\n$3\r\nset\r\n$3\r\nfoo\r\n$3\r\noof\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*3\r\n$8\r\nexpireat\r\n$3\r\nfoo\r\n$10\r\n1282463464\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}

printf '\nexpire\n'
printf '*3\r\n$7\r\npexpire\r\n$3\r\nfoo\r\n$1\r\n0\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*3\r\n$3\r\nset\r\n$3\r\nfoo\r\n$3\r\noof\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*3\r\n$7\r\npexpire\r\n$3\r\nfoo\r\n$1\r\n0\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}

printf '\nrestore\n'
printf '*2\r\n$3\r\ndel\r\n$3\r\nfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$7\r\nrestore\r\n$3\r\nfoo\r\n$1\r\n0\r\n$3\r\noof\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}

printf '\npttl\n'
printf '*2\r\n$4\r\npttl\r\n$3\r\nfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*3\r\n$3\r\nset\r\n$3\r\nfoo\r\n$3\r\noof\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*3\r\n$7\r\npexpire\r\n$3\r\nfoo\r\n$7\r\n1000000\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*2\r\n$4\r\npttl\r\n$3\r\nfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*2\r\n$3\r\ndel\r\n$3\r\nfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}

printf '\nttl\n'
printf '*2\r\n$4\r\npttl\r\n$3\r\nfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*2\r\n$3\r\nttl\r\n$3\r\nfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*3\r\n$3\r\nset\r\n$3\r\nfoo\r\n$3\r\noof\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*3\r\n$6\r\nexpire\r\n$3\r\nfoo\r\n$2\r\n10\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*2\r\n$3\r\nttl\r\n$3\r\nfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}

printf '\ntype\n'
printf '*2\r\n$4\r\ntype\r\n$3\r\nfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*2\r\n$4\r\ntype\r\n$3\r\noof\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}

# strings

printf '\nappend\n'
printf '*3\r\n$6\r\nappend\r\n$3\r\nfoo\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*3\r\n$6\r\nappend\r\n$3\r\n999\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}

printf '\nbitcount\n'
printf '*2\r\n$8\r\nbitcount\r\n$3\r\nfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$8\r\nbitcount\r\n$3\r\nfoo\r\n$1\r\n1\r\n$1\r\n1\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}

printf '\ndecr\n'
printf '*2\r\n$4\r\ndecr\r\n$7\r\ncounter\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}

printf '\ndecrby\n'
printf '*3\r\n$6\r\ndecrby\r\n$7\r\ncounter\r\n$3\r\n100\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}

printf '\nget\n'
printf '*2\r\n$3\r\nget\r\n$16\r\nnon-existent-key\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*2\r\n$3\r\nget\r\n$3\r\nfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*2\r\n$3\r\ndel\r\n$3\r\nfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*2\r\n$3\r\nget\r\n$3\r\nfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}

printf '\ngetbit\n'
printf '*2\r\n$3\r\ndel\r\n$3\r\nfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*3\r\n$6\r\ngetbit\r\n$3\r\nfoo\r\n$1\r\n1\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*3\r\n$3\r\nset\r\n$3\r\nfoo\r\n$3\r\noof\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*3\r\n$6\r\ngetbit\r\n$3\r\nfoo\r\n$1\r\n1\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}

printf '\ngetrange\n'
printf '*2\r\n$3\r\ndel\r\n$3\r\nfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$8\r\ngetrange\r\n$3\r\nfoo\r\n$1\r\n1\r\n$1\r\n2\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*3\r\n$3\r\nset\r\n$3\r\nfoo\r\n$3\r\noof\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$8\r\ngetrange\r\n$3\r\nfoo\r\n$1\r\n1\r\n$1\r\n2\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$8\r\ngetrange\r\n$3\r\nfoo\r\n$1\r\n1\r\n$3\r\n100\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}

printf '\ngetset\n'
printf '*2\r\n$3\r\ndel\r\n$3\r\nfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*3\r\n$6\r\ngetset\r\n$3\r\nfoo\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*2\r\n$3\r\nget\r\n$3\r\nfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}

printf '\nincr\n'
printf '*2\r\n$3\r\ndel\r\n$7\r\ncounter\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*2\r\n$4\r\nincr\r\n$7\r\ncounter\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}

printf '\nincrby\n'
printf '*3\r\n$6\r\nincrby\r\n$7\r\ncounter\r\n$3\r\n100\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}

printf '\nincrbyfloat\n'
printf '*3\r\n$11\r\nincrbyfloat\r\n$7\r\ncounter\r\n$5\r\n10.10\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}

printf '\nmget\n'
printf '*2\r\n$4\r\nmget\r\n$3\r\nfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*3\r\n$3\r\nset\r\n$3\r\nfoo\r\n$3\r\noof\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*2\r\n$4\r\nmget\r\n$3\r\nfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*3\r\n$4\r\nmget\r\n$3\r\nfoo\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*13\r\n$4\r\nmget\r\n$3\r\nfoo\r\n$3\r\nbar\r\n$3\r\nbar\r\n$3\r\nbar\r\n$3\r\nbar\r\n$3\r\nbar\r\n$3\r\nbar\r\n$3\r\nbar\r\n$3\r\nbar\r\n$3\r\nbar\r\n$3\r\nbar\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}

printf '\npsetex\n'
printf '*2\r\n$3\r\ndel\r\n$3\r\nfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$6\r\npsetex\r\n$3\r\nfoo\r\n$4\r\n1000\r\n$3\r\noof\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*2\r\n$3\r\nget\r\n$3\r\nfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}

printf '\nset\n'
printf '*3\r\n$3\r\nset\r\n$3\r\nfoo\r\n$3\r\noof\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}

printf '\nsetbit\n'
printf '*2\r\n$3\r\ndel\r\n$3\r\nfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$6\r\nsetbit\r\n$3\r\nfoo\r\n$1\r\n1\r\n$1\r\n1\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*2\r\n$3\r\nget\r\n$3\r\nfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*3\r\n$3\r\nset\r\n$3\r\nfoo\r\n$3\r\n000\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$6\r\nsetbit\r\n$3\r\nfoo\r\n$1\r\n1\r\n$1\r\n1\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*2\r\n$3\r\nget\r\n$3\r\nfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}

printf '\npsetex\n'
printf '*2\r\n$3\r\ndel\r\n$3\r\nfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$5\r\nsetex\r\n$3\r\nfoo\r\n$4\r\n1000\r\n$3\r\noof\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*2\r\n$3\r\nget\r\n$3\r\nfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}

printf '\nsetnx\n'
printf '*2\r\n$3\r\ndel\r\n$3\r\nfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*3\r\n$5\r\nsetnx\r\n$3\r\nfoo\r\n$3\r\noof\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*2\r\n$3\r\nget\r\n$3\r\nfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*3\r\n$3\r\nset\r\n$3\r\nfoo\r\n$3\r\noof\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*3\r\n$5\r\nsetnx\r\n$3\r\nfoo\r\n$3\r\nooo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*2\r\n$3\r\nget\r\n$3\r\nfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}

printf '\nsetrange\n'
printf '*2\r\n$3\r\ndel\r\n$3\r\nfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$8\r\nsetrange\r\n$3\r\nfoo\r\n$1\r\n1\r\n$3\r\noof\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*2\r\n$3\r\nget\r\n$3\r\nfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$8\r\nsetrange\r\n$3\r\nfoo\r\n$1\r\n4\r\n$3\r\noof\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*2\r\n$3\r\nget\r\n$3\r\nfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}

# hashes

printf '\nhdel\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\nhfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$4\r\nhdel\r\n$4\r\nhfoo\r\n$6\r\nfield1\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$4\r\nhset\r\n$4\r\nhfoo\r\n$6\r\nfield1\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$4\r\nhdel\r\n$4\r\nhfoo\r\n$6\r\nfield1\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}

printf '\nhexists\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\nhfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$4\r\nhdel\r\n$4\r\nhfoo\r\n$6\r\nfield1\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*3\r\n$7\r\nhexists\r\n$4\r\nhfoo\r\n$6\r\nfield1\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$4\r\nhset\r\n$4\r\nhfoo\r\n$6\r\nfield1\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*3\r\n$7\r\nhexists\r\n$4\r\nhfoo\r\n$6\r\nfield1\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}

printf '\nhget\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\nhfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$4\r\nhdel\r\n$4\r\nhfoo\r\n$6\r\nfield1\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*3\r\n$4\r\nhget\r\n$4\r\nhfoo\r\n$6\r\nfield1\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$4\r\nhset\r\n$4\r\nhfoo\r\n$6\r\nfield1\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*3\r\n$4\r\nhget\r\n$4\r\nhfoo\r\n$6\r\nfield1\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}

printf '\nhgetall\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\nhfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*2\r\n$7\r\nhgetall\r\n$4\r\nhfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$4\r\nhset\r\n$4\r\nhfoo\r\n$6\r\nfield1\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$4\r\nhset\r\n$4\r\nhfoo\r\n$6\r\n1dleif\r\n$3\r\nrab\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*2\r\n$7\r\nhgetall\r\n$4\r\nhfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}

printf '\nhincrby\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\nhfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$7\r\nhincrby\r\n$4\r\nhfoo\r\n$6\r\nfield1\r\n$3\r\n100\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*2\r\n$7\r\nhgetall\r\n$4\r\nhfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}

printf '\nhincrbyfloat\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\nhfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$12\r\nhincrbyfloat\r\n$4\r\nhfoo\r\n$6\r\nfield1\r\n$6\r\n100.12\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*2\r\n$7\r\nhgetall\r\n$4\r\nhfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}

printf '\nhkeys\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\nhfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*2\r\n$5\r\nhkeys\r\n$4\r\nhfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$4\r\nhset\r\n$4\r\nhfoo\r\n$6\r\n1dleif\r\n$3\r\nrab\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*2\r\n$5\r\nhkeys\r\n$4\r\nhfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$4\r\nhset\r\n$4\r\nhfoo\r\n$6\r\nfield1\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*2\r\n$5\r\nhkeys\r\n$4\r\nhfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}

printf '\nhlen\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\nhfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*2\r\n$4\r\nhlen\r\n$4\r\nhfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$4\r\nhset\r\n$4\r\nhfoo\r\n$6\r\n1dleif\r\n$3\r\nrab\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$4\r\nhset\r\n$4\r\nhfoo\r\n$6\r\nfield1\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*2\r\n$4\r\nhlen\r\n$4\r\nhfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}

printf '\nhmget\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\nhfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*3\r\n$5\r\nhmget\r\n$4\r\nhfoo\r\n$6\r\nfield1\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$4\r\nhset\r\n$4\r\nhfoo\r\n$6\r\nfield1\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$5\r\nhmget\r\n$4\r\nhfoo\r\n$6\r\nfield1\r\n$6\r\n1dleif\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$4\r\nhset\r\n$4\r\nhfoo\r\n$6\r\n1dleif\r\n$3\r\nrab\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$5\r\nhmget\r\n$4\r\nhfoo\r\n$6\r\nfield1\r\n$6\r\n1dleif\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}

printf '\nhmset\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\nhfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*6\r\n$5\r\nhmset\r\n$4\r\nhfoo\r\n$6\r\nfield1\r\n$3\r\nbar\r\n$6\r\nfield2\r\n$3\r\nbas\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*2\r\n$7\r\nhgetall\r\n$4\r\nhfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}

printf '\nhset\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\nhfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$4\r\nhset\r\n$4\r\nhfoo\r\n$6\r\nfield1\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*2\r\n$7\r\nhgetall\r\n$4\r\nhfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}

printf '\nhsetnx\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\nhfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$6\r\nhsetnx\r\n$4\r\nhfoo\r\n$6\r\nfield1\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*2\r\n$7\r\nhgetall\r\n$4\r\nhfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$6\r\nhsetnx\r\n$4\r\nhfoo\r\n$6\r\nfield1\r\n$3\r\nbas\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*2\r\n$7\r\nhgetall\r\n$4\r\nhfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}

printf '\nhvals\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\nhfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*2\r\n$5\r\nhvals\r\n$4\r\nhfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*6\r\n$5\r\nhmset\r\n$4\r\nhfoo\r\n$6\r\nfield1\r\n$3\r\nbar\r\n$6\r\nfield2\r\n$3\r\nbas\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*2\r\n$5\r\nhvals\r\n$4\r\nhfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}

# lists

printf '\nlindex\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\nlfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*3\r\n$6\r\nlindex\r\n$4\r\nlfoo\r\n$1\r\n1\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*3\r\n$5\r\nlpush\r\n$4\r\nlfoo\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*3\r\n$5\r\nlpush\r\n$4\r\nlfoo\r\n$3\r\nbas\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*3\r\n$6\r\nlindex\r\n$4\r\nlfoo\r\n$1\r\n1\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}

printf '\nlinsert\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\nlfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*3\r\n$5\r\nlpush\r\n$4\r\nlfoo\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*5\r\n$7\r\nlinsert\r\n$4\r\nlfoo\r\n$6\r\nBEFORE\r\n$3\r\nbar\r\n$3\r\nbaq\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*3\r\n$6\r\nlindex\r\n$4\r\nlfoo\r\n$1\r\n0\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}

printf '\nllen\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\nlfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*2\r\n$4\r\nllen\r\n$4\r\nlfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*3\r\n$5\r\nlpush\r\n$4\r\nlfoo\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*3\r\n$5\r\nlpush\r\n$4\r\nlfoo\r\n$3\r\nbas\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*2\r\n$4\r\nllen\r\n$4\r\nlfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}

printf '\nlpop\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\nlfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*2\r\n$4\r\nlpop\r\n$4\r\nlfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$5\r\nlpush\r\n$4\r\nlfoo\r\n$3\r\nbaq\r\n$3\r\nbap\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*2\r\n$4\r\nlpop\r\n$4\r\nlfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*2\r\n$4\r\nlpop\r\n$4\r\nlfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*2\r\n$4\r\nlpop\r\n$4\r\nlfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}

printf '\nlpush\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\nlfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*3\r\n$5\r\nlpush\r\n$4\r\nlfoo\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*3\r\n$5\r\nlpush\r\n$4\r\nlfoo\r\n$3\r\nbas\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$5\r\nlpush\r\n$4\r\nlfoo\r\n$3\r\nbaq\r\n$3\r\nbap\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$6\r\nlrange\r\n$4\r\nlfoo\r\n$1\r\n0\r\n$1\r\n3\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}

printf '\nlpushx\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\nlfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*3\r\n$6\r\nlpushx\r\n$4\r\nlfoo\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*3\r\n$5\r\nlpush\r\n$4\r\nlfoo\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*3\r\n$6\r\nlpushx\r\n$4\r\nlfoo\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*3\r\n$6\r\nlpushx\r\n$4\r\nlfoo\r\n$3\r\nbas\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$6\r\nlrange\r\n$4\r\nlfoo\r\n$1\r\n0\r\n$1\r\n3\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}

printf '\nlrange\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\nlfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$6\r\nlrange\r\n$4\r\nlfoo\r\n$1\r\n0\r\n$1\r\n2\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*6\r\n$5\r\nlpush\r\n$4\r\nlfoo\r\n$3\r\nbar\r\n$3\r\nbas\r\n$3\r\nbat\r\n$3\r\nbau\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$6\r\nlrange\r\n$4\r\nlfoo\r\n$1\r\n0\r\n$1\r\n2\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}

printf '\nlrem\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\nlfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$4\r\nlrem\r\n$4\r\nlfoo\r\n$1\r\n2\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*6\r\n$5\r\nlpush\r\n$4\r\nlfoo\r\n$3\r\nbar\r\n$3\r\nbar\r\n$3\r\nbar\r\n$3\r\nbau\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$4\r\nlrem\r\n$4\r\nlfoo\r\n$1\r\n2\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$4\r\nlrem\r\n$4\r\nlfoo\r\n$1\r\n2\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}

printf '\nlset\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\nlfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$4\r\nlset\r\n$4\r\nlfoo\r\n$1\r\n1\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*3\r\n$5\r\nlpush\r\n$4\r\nlfoo\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$4\r\nlset\r\n$4\r\nlfoo\r\n$1\r\n0\r\n$3\r\nbaq\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$4\r\nlset\r\n$4\r\nlfoo\r\n$1\r\n1\r\n$3\r\nbas\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$6\r\nlrange\r\n$4\r\nlfoo\r\n$1\r\n0\r\n$1\r\n2\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}

printf '\nltrim\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\nlfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$5\r\nltrim\r\n$4\r\nlfoo\r\n$1\r\n0\r\n$1\r\n2\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*5\r\n$5\r\nlpush\r\n$4\r\nlfoo\r\n$3\r\nbaq\r\n$3\r\nbap\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}

printf '\nrpop\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\nlfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*2\r\n$4\r\nrpop\r\n$4\r\nlfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$5\r\nlpush\r\n$4\r\nlfoo\r\n$3\r\nbaq\r\n$3\r\nbap\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*2\r\n$4\r\nrpop\r\n$4\r\nlfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*2\r\n$4\r\nrpop\r\n$4\r\nlfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*2\r\n$4\r\nrpop\r\n$4\r\nlfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}

printf '\nrpoplpush\n'
printf '*2\r\n$3\r\ndel\r\n$6\r\n{lfoo}\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*2\r\n$3\r\ndel\r\n$7\r\n{lfoo}2\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*5\r\n$5\r\nlpush\r\n$6\r\n{lfoo}\r\n$3\r\nbar\r\n$3\r\nbaq\r\n$3\r\nbap\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*3\r\n$9\r\nrpoplpush\r\n$6\r\n{lfoo}\r\n$7\r\n{lfoo}2\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*3\r\n$9\r\nrpoplpush\r\n$6\r\n{lfoo}\r\n$7\r\n{lfoo}2\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$6\r\nlrange\r\n$6\r\n{lfoo}\r\n$1\r\n0\r\n$1\r\n3\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$6\r\nlrange\r\n$7\r\n{lfoo}2\r\n$1\r\n0\r\n$1\r\n3\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}

printf '\nrpush\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\nlfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*3\r\n$5\r\nrpush\r\n$4\r\nlfoo\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*3\r\n$5\r\nrpush\r\n$4\r\nlfoo\r\n$3\r\nbas\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$5\r\nrpush\r\n$4\r\nlfoo\r\n$3\r\nbat\r\n$3\r\nbau\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$6\r\nlrange\r\n$4\r\nlfoo\r\n$1\r\n0\r\n$1\r\n3\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}

printf '\nrpushx\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\nlfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*3\r\n$6\r\nrpushx\r\n$4\r\nlfoo\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*3\r\n$5\r\nrpush\r\n$4\r\nlfoo\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*3\r\n$6\r\nrpushx\r\n$4\r\nlfoo\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*3\r\n$6\r\nrpushx\r\n$4\r\nlfoo\r\n$3\r\nbas\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$6\r\nlrange\r\n$4\r\nlfoo\r\n$1\r\n0\r\n$1\r\n3\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}

# sets

printf '\nsadd\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\nsfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$4\r\nsadd\r\n$4\r\nsfoo\r\n$3\r\nbar\r\n$3\r\nbas\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*2\r\n$8\r\nsmembers\r\n$4\r\nsfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}

printf '\nscard\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\nsfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*2\r\n$5\r\nscard\r\n$4\r\nsfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$4\r\nsadd\r\n$4\r\nsfoo\r\n$3\r\nbar\r\n$3\r\nbas\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*2\r\n$5\r\nscard\r\n$4\r\nsfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}

printf '\nsdiff\n'
printf '*2\r\n$3\r\ndel\r\n$6\r\n{sfoo}\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*2\r\n$3\r\ndel\r\n$7\r\n{sfoo}2\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$4\r\nsadd\r\n$6\r\n{sfoo}\r\n$3\r\nbar\r\n$3\r\nbas\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*3\r\n$4\r\nsadd\r\n$7\r\n{sfoo}2\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*3\r\n$5\r\nsdiff\r\n$6\r\n{sfoo}\r\n$7\r\n{sfoo}2\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}

printf '\nsdiffstore\n'
printf '*2\r\n$3\r\ndel\r\n$6\r\n{sfoo}\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*2\r\n$3\r\ndel\r\n$7\r\n{sfoo}2\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*2\r\n$3\r\ndel\r\n$7\r\n{sfoo}3\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$4\r\nsadd\r\n$6\r\n{sfoo}\r\n$3\r\nbar\r\n$3\r\nbas\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*3\r\n$4\r\nsadd\r\n$7\r\n{sfoo}2\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$10\r\nsdiffstore\r\n$7\r\n{sfoo}3\r\n$6\r\n{sfoo}\r\n$7\r\n{sfoo}2\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*2\r\n$8\r\nsmembers\r\n$7\r\n{sfoo}3\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}

printf '\nsinter\n'
printf '*2\r\n$3\r\ndel\r\n$6\r\n{sfoo}\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*2\r\n$3\r\ndel\r\n$7\r\n{sfoo}2\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$4\r\nsadd\r\n$6\r\n{sfoo}\r\n$3\r\nbar\r\n$3\r\nbas\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*3\r\n$4\r\nsadd\r\n$7\r\n{sfoo}2\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*3\r\n$6\r\nsinter\r\n$6\r\n{sfoo}\r\n$7\r\n{sfoo}2\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}

printf '\nsinterstore\n'
printf '*2\r\n$3\r\ndel\r\n$6\r\n{sfoo}\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*2\r\n$3\r\ndel\r\n$7\r\n{sfoo}2\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*2\r\n$3\r\ndel\r\n$7\r\n{sfoo}3\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$4\r\nsadd\r\n$6\r\n{sfoo}\r\n$3\r\nbar\r\n$3\r\nbas\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*3\r\n$4\r\nsadd\r\n$7\r\n{sfoo}2\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$11\r\nsinterstore\r\n$7\r\n{sfoo}3\r\n$6\r\n{sfoo}\r\n$7\r\n{sfoo}2\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*2\r\n$8\r\nsmembers\r\n$7\r\n{sfoo}3\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}

printf '\nsismember\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\nsfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*3\r\n$9\r\nsismember\r\n$4\r\nsfoo\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$4\r\nsadd\r\n$4\r\nsfoo\r\n$3\r\nbar\r\n$3\r\nbas\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*3\r\n$9\r\nsismember\r\n$4\r\nsfoo\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*3\r\n$9\r\nsismember\r\n$4\r\nsfoo\r\n$3\r\nrab\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}

printf '\nsmembers\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\nsfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*2\r\n$8\r\nsmembers\r\n$4\r\nsfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$4\r\nsadd\r\n$4\r\nsfoo\r\n$3\r\nbar\r\n$3\r\nbas\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*2\r\n$8\r\nsmembers\r\n$4\r\nsfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}

printf '\nsmove\n'
printf '*2\r\n$3\r\ndel\r\n$6\r\n{sfoo}\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*2\r\n$3\r\ndel\r\n$7\r\n{sfoo}2\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$4\r\nsadd\r\n$6\r\n{sfoo}\r\n$3\r\nbar\r\n$3\r\nbas\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$5\r\nsmove\r\n$6\r\n{sfoo}\r\n$7\r\n{sfoo}2\r\n$3\r\nbas\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*2\r\n$8\r\nsmembers\r\n$6\r\n{sfoo}\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*2\r\n$8\r\nsmembers\r\n$7\r\n{sfoo}2\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}

printf '\nspop\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\nsfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*2\r\n$4\r\nspop\r\n$4\r\nsfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$4\r\nsadd\r\n$4\r\nsfoo\r\n$3\r\nbar\r\n$3\r\nbas\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*2\r\n$4\r\nspop\r\n$4\r\nsfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}

printf '\nsrandmember\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\nsfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*2\r\n$11\r\nsrandmember\r\n$4\r\nsfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$4\r\nsadd\r\n$4\r\nsfoo\r\n$3\r\nbar\r\n$3\r\nbas\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*2\r\n$11\r\nsrandmember\r\n$4\r\nsfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*2\r\n$11\r\nsrandmember\r\n$4\r\nsfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*2\r\n$11\r\nsrandmember\r\n$4\r\nsfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*3\r\n$11\r\nsrandmember\r\n$4\r\nsfoo\r\n$1\r\n2\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}

printf '\nsrem\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\nsfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*3\r\n$4\r\nsrem\r\n$4\r\nsfoo\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*5\r\n$4\r\nsadd\r\n$4\r\nsfoo\r\n$3\r\nbar\r\n$3\r\nbas\r\n$3\r\nbat\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*3\r\n$4\r\nsrem\r\n$4\r\nsfoo\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*5\r\n$4\r\nsrem\r\n$4\r\nsfoo\r\n$3\r\nbas\r\n$3\r\nbat\r\n$3\r\nrab\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}

printf '\nsunion\n'
printf '*2\r\n$3\r\ndel\r\n$6\r\n{sfoo}\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*2\r\n$3\r\ndel\r\n$7\r\n{sfoo}2\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$4\r\nsadd\r\n$6\r\n{sfoo}\r\n$3\r\nbar\r\n$3\r\nbas\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*3\r\n$4\r\nsadd\r\n$7\r\n{sfoo}2\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*3\r\n$6\r\nsunion\r\n$6\r\n{sfoo}\r\n$7\r\n{sfoo}2\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}

printf '\nsunionstore\n'
printf '*2\r\n$3\r\ndel\r\n$6\r\n{sfoo}\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*2\r\n$3\r\ndel\r\n$7\r\n{sfoo}2\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*2\r\n$3\r\ndel\r\n$7\r\n{sfoo}3\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$4\r\nsadd\r\n$6\r\n{sfoo}\r\n$3\r\nbar\r\n$3\r\nbas\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*3\r\n$4\r\nsadd\r\n$7\r\n{sfoo}2\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$11\r\nsunionstore\r\n$7\r\n{sfoo}3\r\n$6\r\n{sfoo}\r\n$7\r\n{sfoo}2\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*2\r\n$8\r\nsmembers\r\n$7\r\n{sfoo}3\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}

# sorted sets

printf '\nzadd\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\nzfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$4\r\nzadd\r\n$4\r\nzfoo\r\n$3\r\n100\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$4\r\nzadd\r\n$4\r\nzfoo\r\n$3\r\n100\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*6\r\n$4\r\nzadd\r\n$4\r\nzfoo\r\n$3\r\n101\r\n$3\r\nbat\r\n$3\r\n102\r\n$3\r\nbau\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}

printf '\nzcard\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\nzfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*2\r\n$5\r\nzcard\r\n$4\r\nzfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*8\r\n$4\r\nzadd\r\n$4\r\nzfoo\r\n$3\r\n100\r\n$3\r\nbar\r\n$3\r\n101\r\n$3\r\nbat\r\n$3\r\n102\r\n$3\r\nbau\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*2\r\n$5\r\nzcard\r\n$4\r\nzfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}

printf '\nzcount\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\nzfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$6\r\nzcount\r\n$4\r\nzfoo\r\n$3\r\n100\r\n$3\r\n101\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*8\r\n$4\r\nzadd\r\n$4\r\nzfoo\r\n$3\r\n100\r\n$3\r\nbar\r\n$3\r\n101\r\n$3\r\nbat\r\n$3\r\n102\r\n$3\r\nbau\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$6\r\nzcount\r\n$4\r\nzfoo\r\n$3\r\n100\r\n$3\r\n101\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$6\r\nzcount\r\n$4\r\nzfoo\r\n$4\r\n-inf\r\n$4\r\n+inf\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}

printf '\nzincrby\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\nzfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$7\r\nzincrby\r\n$4\r\nzfoo\r\n$3\r\n100\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$7\r\nzincrby\r\n$4\r\nzfoo\r\n$3\r\n100\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}

printf '\nzinterstore\n'
printf '*2\r\n$3\r\ndel\r\n$6\r\n{zfoo}\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*2\r\n$3\r\ndel\r\n$7\r\n{zfoo}2\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*2\r\n$3\r\ndel\r\n$7\r\n{zfoo}3\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*8\r\n$4\r\nzadd\r\n$6\r\n{zfoo}\r\n$3\r\n100\r\n$3\r\nbar\r\n$3\r\n101\r\n$3\r\nbat\r\n$3\r\n102\r\n$3\r\nbau\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*6\r\n$4\r\nzadd\r\n$7\r\n{zfoo}2\r\n$3\r\n100\r\n$3\r\nbar\r\n$3\r\n101\r\n$3\r\nbat\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*5\r\n$11\r\nzinterstore\r\n$7\r\n{zfoo}3\r\n$1\r\n2\r\n$6\r\n{zfoo}\r\n$7\r\n{zfoo}2\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*5\r\n$6\r\nzrange\r\n$7\r\n{zfoo}3\r\n$1\r\n0\r\n$1\r\n3\r\n$10\r\nWITHSCORES\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}

printf '\nzrange\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\nzfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$6\r\nzrange\r\n$4\r\nzfoo\r\n$1\r\n0\r\n$1\r\n3\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*8\r\n$4\r\nzadd\r\n$4\r\nzfoo\r\n$3\r\n100\r\n$3\r\nbar\r\n$3\r\n101\r\n$3\r\nbat\r\n$3\r\n102\r\n$3\r\nbau\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$6\r\nzrange\r\n$4\r\nzfoo\r\n$1\r\n0\r\n$1\r\n3\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*5\r\n$6\r\nzrange\r\n$4\r\nzfoo\r\n$1\r\n0\r\n$1\r\n3\r\n$10\r\nWITHSCORES\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}

printf '\nzrangebyscore\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\nzfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$13\r\nzrangebyscore\r\n$4\r\nzfoo\r\n$3\r\n100\r\n$3\r\n101\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*8\r\n$4\r\nzadd\r\n$4\r\nzfoo\r\n$3\r\n100\r\n$3\r\nbar\r\n$3\r\n101\r\n$3\r\nbat\r\n$3\r\n102\r\n$3\r\nbau\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$13\r\nzrangebyscore\r\n$4\r\nzfoo\r\n$3\r\n100\r\n$3\r\n101\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}

printf '\nzrank\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\nzfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*3\r\n$5\r\nzrank\r\n$4\r\nzfoo\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*8\r\n$4\r\nzadd\r\n$4\r\nzfoo\r\n$3\r\n100\r\n$3\r\nbar\r\n$3\r\n101\r\n$3\r\nbat\r\n$3\r\n102\r\n$3\r\nbau\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*3\r\n$5\r\nzrank\r\n$4\r\nzfoo\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}

printf '\nzrem\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\nzfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*3\r\n$4\r\nzrem\r\n$4\r\nzfoo\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*8\r\n$4\r\nzadd\r\n$4\r\nzfoo\r\n$3\r\n100\r\n$3\r\nbar\r\n$3\r\n101\r\n$3\r\nbat\r\n$3\r\n102\r\n$3\r\nbau\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*8\r\n$4\r\nzrem\r\n$4\r\nzfoo\r\n$3\r\n100\r\n$3\r\nbar\r\n$3\r\n101\r\n$3\r\nbat\r\n$3\r\n102\r\n$3\r\nbau\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}

printf '\nzremrangebyrank\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\nzfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$15\r\nzremrangebyrank\r\n$4\r\nzfoo\r\n$1\r\n0\r\n$1\r\n1\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*8\r\n$4\r\nzadd\r\n$4\r\nzfoo\r\n$3\r\n100\r\n$3\r\nbar\r\n$3\r\n101\r\n$3\r\nbat\r\n$3\r\n102\r\n$3\r\nbau\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$15\r\nzremrangebyrank\r\n$4\r\nzfoo\r\n$1\r\n0\r\n$1\r\n1\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}

printf '\nzremrangebyscore\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\nzfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$16\r\nzremrangebyscore\r\n$4\r\nzfoo\r\n$3\r\n100\r\n$3\r\n101\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*8\r\n$4\r\nzadd\r\n$4\r\nzfoo\r\n$3\r\n100\r\n$3\r\nbar\r\n$3\r\n101\r\n$3\r\nbat\r\n$3\r\n102\r\n$3\r\nbau\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$16\r\nzremrangebyscore\r\n$4\r\nzfoo\r\n$3\r\n100\r\n$3\r\n101\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}

printf '\nzrevrange\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\nzfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$9\r\nzrevrange\r\n$4\r\nzfoo\r\n$1\r\n0\r\n$1\r\n2\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*8\r\n$4\r\nzadd\r\n$4\r\nzfoo\r\n$3\r\n100\r\n$3\r\nbar\r\n$3\r\n101\r\n$3\r\nbat\r\n$3\r\n102\r\n$3\r\nbau\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$9\r\nzrevrange\r\n$4\r\nzfoo\r\n$1\r\n0\r\n$1\r\n2\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}

printf '\nzrevrangebyscore\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\nzfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$16\r\nzrevrangebyscore\r\n$4\r\nzfoo\r\n$3\r\n101\r\n$3\r\n100\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*8\r\n$4\r\nzadd\r\n$4\r\nzfoo\r\n$3\r\n100\r\n$3\r\nbar\r\n$3\r\n101\r\n$3\r\nbat\r\n$3\r\n102\r\n$3\r\nbau\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$16\r\nzrevrangebyscore\r\n$4\r\nzfoo\r\n$3\r\n101\r\n$3\r\n100\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}

printf '\nzrevrank\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\nzfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*3\r\n$8\r\nzrevrank\r\n$4\r\nzfoo\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*8\r\n$4\r\nzadd\r\n$4\r\nzfoo\r\n$3\r\n100\r\n$3\r\nbar\r\n$3\r\n101\r\n$3\r\nbat\r\n$3\r\n102\r\n$3\r\nbau\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*3\r\n$8\r\nzrevrank\r\n$4\r\nzfoo\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}

printf '\nzscore\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\nzfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*3\r\n$6\r\nzscore\r\n$4\r\nzfoo\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*8\r\n$4\r\nzadd\r\n$4\r\nzfoo\r\n$3\r\n100\r\n$3\r\nbar\r\n$3\r\n101\r\n$3\r\nbat\r\n$3\r\n102\r\n$3\r\nbau\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*3\r\n$6\r\nzscore\r\n$4\r\nzfoo\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}

printf '\nzunionstore\n'
printf '*2\r\n$3\r\ndel\r\n$6\r\n{zfoo}\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*2\r\n$3\r\ndel\r\n$7\r\n{zfoo}2\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*2\r\n$3\r\ndel\r\n$7\r\n{zfoo}3\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*8\r\n$4\r\nzadd\r\n$6\r\n{zfoo}\r\n$3\r\n100\r\n$3\r\nbar\r\n$3\r\n101\r\n$3\r\nbat\r\n$3\r\n102\r\n$3\r\nbau\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*6\r\n$4\r\nzadd\r\n$7\r\n{zfoo}2\r\n$3\r\n100\r\n$3\r\nbar\r\n$3\r\n101\r\n$3\r\nbat\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*5\r\n$11\r\nzunionstore\r\n$7\r\n{zfoo}3\r\n$1\r\n2\r\n$6\r\n{zfoo}\r\n$7\r\n{zfoo}2\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*5\r\n$6\r\nzrange\r\n$7\r\n{zfoo}3\r\n$1\r\n0\r\n$1\r\n3\r\n$10\r\nWITHSCORES\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}

printf '\nzlexcount\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\nzfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$9\r\nzlexcount\r\n$4\r\nzfoo\r\n$2\r\n(a\r\n$2\r\n(z\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*8\r\n$4\r\nzadd\r\n$4\r\nzfoo\r\n$3\r\n100\r\n$3\r\nbar\r\n$3\r\n101\r\n$3\r\nbat\r\n$3\r\n102\r\n$3\r\nbau\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$9\r\nzlexcount\r\n$4\r\nzfoo\r\n$2\r\n(a\r\n$4\r\n[bat\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$9\r\nzlexcount\r\n$4\r\nzfoo\r\n$1\r\n-\r\n$1\r\n+\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}

printf '\nzrangebylex\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\nzfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$11\r\nzrangebylex\r\n$4\r\nzfoo\r\n$2\r\n(a\r\n$2\r\n(z\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*8\r\n$4\r\nzadd\r\n$4\r\nzfoo\r\n$3\r\n100\r\n$3\r\nbar\r\n$3\r\n101\r\n$3\r\nbat\r\n$3\r\n102\r\n$3\r\nbau\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$11\r\nzrangebylex\r\n$4\r\nzfoo\r\n$2\r\n(a\r\n$4\r\n[bat\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$11\r\nzrangebylex\r\n$4\r\nzfoo\r\n$1\r\n-\r\n$1\r\n+\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}

printf '\nzremrangebylex\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\nzfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$14\r\nzremrangebylex\r\n$4\r\nzfoo\r\n$2\r\n(a\r\n$2\r\n(z\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*8\r\n$4\r\nzadd\r\n$4\r\nzfoo\r\n$3\r\n100\r\n$3\r\nbar\r\n$3\r\n101\r\n$3\r\nbat\r\n$3\r\n102\r\n$3\r\nbau\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$14\r\nzremrangebylex\r\n$4\r\nzfoo\r\n$2\r\n(a\r\n$2\r\n(z\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$14\r\nzremrangebylex\r\n$4\r\nzfoo\r\n$1\r\n-\r\n$1\r\n+\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}

# hyperloglog

printf '\npfadd\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\npfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$5\r\npfadd\r\n$4\r\npfoo\r\n$3\r\nbar\r\n$3\r\nbas\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*2\r\n$7\r\npfcount\r\n$4\r\npfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}

printf '\npfcount\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\npfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*2\r\n$7\r\npfcount\r\n$4\r\npfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$5\r\npfadd\r\n$4\r\npfoo\r\n$3\r\nbar\r\n$3\r\nbas\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*2\r\n$7\r\npfcount\r\n$4\r\npfoo\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}

printf '\npfmerge\n'
printf '*2\r\n$3\r\ndel\r\n$6\r\n{pfoo}\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*2\r\n$3\r\ndel\r\n$7\r\n{pfoo}2\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*2\r\n$3\r\ndel\r\n$7\r\n{pfoo}3\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$5\r\npfadd\r\n$6\r\n{pfoo}\r\n$4\r\nsfoo\r\n$3\r\nbar\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*3\r\n$5\r\npfadd\r\n$7\r\n{pfoo}2\r\n$3\r\nbas\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*5\r\n$7\r\npfmerge\r\n$7\r\n{pfoo}3\r\n$1\r\n2\r\n$6\r\n{pfoo}\r\n$7\r\n{pfoo}2\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*2\r\n$7\r\npfcount\r\n$7\r\n{pfoo}3\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}

# scripting

printf '\neval\n'
printf '*2\r\n$4\r\neval\r\n$10\r\nreturn 123\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*3\r\n$4\r\neval\r\n$10\r\nreturn 123\r\n$1\r\n0\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*3\r\n$4\r\neval\r\n$10\r\nreturn 123\r\n$1\r\n1\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$4\r\neval\r\n$10\r\nreturn 123\r\n$1\r\n1\r\n$1\r\n1\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*7\r\n$4\r\neval\r\n$40\r\nreturn {KEYS[1],KEYS[2],ARGV[1],ARGV[2]}\r\n$1\r\n2\r\n$9\r\nkey1{tag}\r\n$4\r\narg1\r\n$9\r\nkey2{tag}\r\n$4\r\narg2\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*9\r\n$4\r\neval\r\n$56\r\nreturn {KEYS[1],KEYS[2],KEYS[3],ARGV[1],ARGV[2],ARGV[3]}\r\n$1\r\n3\r\n$9\r\nkey1{tag}\r\n$4\r\narg1\r\n$9\r\nkey2{tag}\r\n$4\r\narg2\r\n$9\r\nkey3{tag}\r\n$4\r\narg3\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*4\r\n$4\r\neval\r\n$11\r\nreturn {10}\r\n$1\r\n1\r\n$4\r\nTEMP\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}

printf '\nsaddint\n'
printf '*2\r\n$3\r\ndel\r\n$4\r\nsint\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*5\r\n$7\r\nsaddint\r\n$4\r\nsint\r\n$1\r\n1\r\n$1\r\n2\r\n$1\r\n3\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*2\r\n$8\r\nsmembers\r\n$4\r\nsint\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}

printf '\nobject\n'
printf '*3\r\n$6\r\nobject\r\n$8\r\nencoding\r\n$4\r\nsint\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*3\r\n$6\r\nobject\r\n$8\r\nrefcount\r\n$4\r\nsint\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*3\r\n$6\r\nobject\r\n$8\r\nidletime\r\n$4\r\nsint\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
printf '*3\r\n$6\r\nobject\r\n$4\r\nfreq\r\n$4\r\nsint\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}

# quit
printf '\nquit\n'
printf '*1\r\n$4\r\nquit\r\n' | socat ${debug} ${timeout} - TCP:localhost:${port}
