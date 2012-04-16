# IP filtering

## Introduction

## Make a new ip table

    ip_tables.insert_table = <table_name>

Create a new empty table with the name ’table\_name’, with the default
value returned being $0$.

There is currently no use of the generic ip tables commands.

## Add a new address block

    ip_tables.add_address = <table_name>, 10.0.0.0/8, <value>

Set <value\> for all addresses in the address block, overwriting prior
values.

## Add a new address block

    ip_tables.load = <table_name>, ~/foo.txt, <value>

Set <value\> for all addresses in the file ’foo.txt’ separated by
newline, similar to ’add\_address’.

## Get value for address

    ip_tables.get = <table_name>, 10.10.10.10

Returns the value set for an address, or the address block it belongs
to. The default is $0$.

## Size of data structures

    ip_tables.size_data = <table_name>

Returns the size in bytes of all data structures for this table,
excluding the root class object itself. Note that the in-memory table is
dynamically consolidated, as such memory use will always be based on
actual fragmentation.

The table is a b-tree with 1024 nodes per branch.

## IPv4 filtering table

    ipv4_filter.add_address = 10.0.0.0/8, unwanted
    ipv4_filter.add_address = 11.0.0.0/8, preferred
    ipv4_filter.load = ~/filters.txt, unwanted
    ipv4_filter.get = 10.10.10.10
    ipv4_filter.size_data =

The main ip filter, currently supporting ’unwanted’ (do not allow
connections) and ’preferred’ (currently used only in private code).

## Constants

    strings.ip_filter =
    =>
    { "unwanted",  PeerInfo::flag_unwanted },
    { "preferred", PeerInfo::flag_preferred },

Constants used by ipv4\_filter values.
