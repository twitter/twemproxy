#!/usr/bin/env php
<?php
/**
 * @author Tyson Andre
 *
 * Heuristics to extract commands from redis-doc and determine what group of commands
 * they'd fall under for twemproxy's request parsing logic.
 */

if (count($argv) !== 2) {
    echo "Usage: ${argv[0]} commands.json\n";
    echo "commands.json can be downloaded from https://github.com/redis/redis-doc\n";
    exit(1);
}
$path = $argv[1];
$contents = file_get_contents($path);
if (!is_string($contents)) {
    echo "Failed to read $path\n";
    exit(1);
}
$commands = json_decode($contents, true);
uasort($commands, fn($a, $b) => version_compare($b['since'], $a['since']));

const INFINITE_ARGS = 100000;

function categorize_arg(array $arg, string $commandName): array {
    $min = 1;
    $max = 1;
    if ($arg['multiple']) {
        $min = 0;
        $max = INFINITE_ARGS;
    }
    if ($arg['optional']) {
        $min = 0;
    }
    if ($arg['type'] === 'key') {
        return ['min_key' => $min, 'max_key' => $max];
    }
    return ['min_arg' => $min, 'max_arg' => $max];
}

function categorize(array $command, string $commandName): string {
    $minKeyCount = 0;
    $maxKeyCount = 0;
    $minArgCount = 0;
    $maxArgCount = 0;
    $arguments = $command['arguments'] ?? [];
    foreach ($arguments as $arg) {
        $data = categorize_arg($arg, $commandName);
        $minKeyCount += ($data['min_key'] ?? 0);
        $maxKeyCount += ($data['max_key'] ?? 0);
        $minArgCount += ($data['min_arg'] ?? 0);
        $maxArgCount += ($data['max_arg'] ?? 0);
    }
    if (in_array($commandName, ['DEL', 'MGET', 'MSET', 'TOUCH', 'UNLINK'])) {
        return "keyn";
    }
    if ($maxKeyCount > $minKeyCount || $maxArgCount > $minArgCount) {
        // return "key${minKeyCount}_argx";
        return "key1_argx";
    }
    // Assume that
    // min=max for arg and key
    if ($minArgCount > 0 && $minKeyCount >= 2) {
        return "key1_arg" . ($minArgCount + $maxKeyCount - 1);
    }
    return "key${minKeyCount}_arg" . $minArgCount;
}

const KEY1 = [
    'PERSIST',
    'PTTL',
    'TTL',
    'TYPE',
    'DUMP',

    'DECR',
    'GET',
    'GETDEL',
    'INCR',
    'STRLEN',

    'HGETALL',
    'HKEYS',
    'HLEN',
    'HVALS',

    'LLEN',

    'SCARD',
    'SMEMBERS',

    'ZCARD',
    // 'AUTH',
];

const KEY1_ARG1 = [
    'EXPIRE',
    'EXPIREAT',
    'PEXPIRE',
    'PEXPIREAT',
    'MOVE',

    'APPEND',
    'DECRBY',
    'GETBIT',
    'GETSET',
    'INCRBY',
    'INCRBYFLOAT',
    'SETNX',

    'HEXISTS',
    'HGET',
    'HSTRLEN',

    'LINDEX',
    'RPOPLPUSH',

    'SISMEMBER',

    'ZRANK',
    'ZREVRANK',
    'ZSCORE',
];

const KEY1_ARG2 = [
    'GETRANGE',
    'PSETEX',
    'SETBIT',
    'SETEX',
    'SETRANGE',

    'HINCRBY',
    'HINCRBYFLOAT',
    'HSETNX',

    'LRANGE',
    'LREM',
    'LSET',
    'LTRIM',

    'SMOVE',

    'ZCOUNT',
    'ZLEXCOUNT',
    'ZINCRBY',
    'ZREMRANGEBYLEX',
    'ZREMRANGEBYRANK',
    'ZREMRANGEBYSCORE',
];

const KEY1_ARG3 = [
    'LINSERT',
    'LMOVE',
];

const KEY1_ARGN = [
    'SORT',

    'BITCOUNT',
    'BITPOS',
    'BITFIELD',
    'BITOP',

    'EXISTS',
    'GETEX',
    'SET',

    'HDEL',
    'HMGET',
    'HMSET',
    'HSCAN',
    'HSET',
    'HRANDFIELD',

    'LPUSH',
    'LPUSHX',
    'RPUSH',
    'RPUSHX',
    'LPOP',
    'RPOP',
    'LPOS',

    'SADD',
    'SDIFF',
    'SDIFFSTORE',
    'SINTER',
    'SINTERSTORE',
    'SREM',
    'SUNION',
    'SUNIONSTORE',
    'SRANDMEMBER',
    'SSCAN',
    'SPOP',
    'SMISMEMBER',

    'PFADD',
    'PFMERGE',
    'PFCOUNT',

    'ZADD',
    'ZDIFF',
    'ZDIFFSTORE',
    'ZINTER',
    'ZINTERSTORE',
    'ZMSCORE',
    'ZPOPMAX',
    'ZPOPMIN',
    'ZRANDMEMBER',
    'ZRANGE',
    'ZRANGEBYLEX',
    'ZRANGEBYSCORE',
    'ZRANGESTORE',
    'ZREM',
    'ZREVRANGE',
    'ZREVRANGEBYLEX',
    'ZREVRANGEBYSCORE',
    'ZSCAN',
    'ZUNION',
    'ZUNIONSTORE',

    'GEODIST',
    'GEOPOS',
    'GEOHASH',
    'GEOADD',
    'GEOSEARCH',

    'RESTORE',
];

const EXPECTED_MAPS = [
    'key1_arg0' => KEY1,
    'key1_arg1' => KEY1_ARG1,
    'key1_arg2' => KEY1_ARG2,
    'key1_arg3' => KEY1_ARG3,
    'key1_argx' => KEY1_ARGN,
];

function compute_types(): array {
    global $commands;
    $types = [];
    foreach ($commands as $name => $cmd) {
        // printf("%s: %s\n", $name, json_encode($cmd, JSON_PRETTY_PRINT));
        try {
            $type = categorize($cmd, $name);
        } catch (Exception $e) {
            $type = "unknown: {$e->getMessage()} " . json_encode($cmd);
        }
        $types[$name] = $type;
    }
    return $types;
}

function dump_mismatched_argument_types(array $types, array $commands): void {
    foreach (EXPECTED_MAPS as $expected => $maps) {
        foreach ($maps as $key) {
            $actual = $types[$key];
            if ($actual !== $expected) {
                echo "Unexpected type for $key: got $actual, want $expected: " . json_encode($commands[$key]['arguments']) . "\n";
            }
        }
        foreach ($types as $other_name => $type) {
            if ($type === $expected && !in_array($other_name, $maps)) {
                $command = $commands[$other_name];
                echo "Expected $other_name in $expected: " . json_encode($command['arguments']) . "\n";
                echo "> " . $command['group'] . ": " . $command['summary'] . "\n\n";
            }
        }
    }
}

function render_arg(array $argument): string {
    if ($argument['optional'] ?? false) {
        unset($argument['optional']);
        return '[' . render_arg($argument) . ']';
    }
    if ($argument['enum'] ?? null) {
        return implode('|', $argument['enum']);
    }
    if ($argument['command'] ?? null) {
        return $argument['command'];
    }
    $name = $argument['name'];

    $repr = is_array($name) ? implode(' ', $name) : $name;
    if ($argument['multiple'] ?? false) {
        return "$repr [$repr …]";
    }
    return $repr;
}

function render_command(string $name, array $command): string {
    $repr = $name;
    foreach ($command['arguments'] ?? [] as $argument) {
        $repr .= ' ' . render_arg($argument);
    }
    return $repr;
}

function center_pad(string $name, int $len) {
    if (mb_strlen($name) >= $len) {
        return $name;
    }
    $name = str_repeat(' ', ($len - mb_strlen($name)) >> 1) . $name;
    $name .= str_repeat(' ', $len - mb_strlen($name));
    return $name;
}

function right_pad(string $name, int $len) {
    if (mb_strlen($name) >= $len) {
        return $name;
    }
    $name .= str_repeat(' ', $len - mb_strlen($name));
    return $name;
}

function dump_table(array $commands) {
    $header = <<<EOT
    +-------------------+------------+---------------------------------------------------------------------------------------------------------------------+
    |      Command      | Supported? | Format                                                                                                              |
    +-------------------+------------+---------------------------------------------------------------------------------------------------------------------+
EOT;
    echo $header . "\n";
    $rowLine = explode("\n", $header)[0];
    ksort($commands);
    $parts = explode('+', $rowLine);
    $nameLen = strlen($parts[1]);
    $supportsLen = strlen($parts[2]);
    $commandLen = strlen($parts[3]);
    foreach ($commands as $name => $command) {
        $key = center_pad($name, 19);
        $commandRepr = render_command($name, $command);
        $supports = 'Yes';
        printf("    |%s|%s|%s|\n", center_pad($name, $nameLen), center_pad($supports, $supportsLen), right_pad(' ' . $commandRepr, $commandLen));
        echo $rowLine . "\n";

    }
    echo "\n";
}

function dump_table_groups(array $commands): void {
    $groups = [];
    foreach ($commands as $name => $command) {
        $groups[$command['group']][$name] = $command;
    }
    foreach ($groups as $groupName => $group) {
        printf("### %s Command\n\n", $groupName);

        dump_table($group);
    }
}

$types = compute_types();
foreach ($types as $name => $type) {
    printf("%s: %s\n", $name, $type);
}

dump_mismatched_argument_types($types, $commands);
dump_table_groups($commands);
