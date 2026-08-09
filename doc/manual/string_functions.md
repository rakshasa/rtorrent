# String functions

The `string.*` commands manipulate and inspect text from within the
configuration file.
They are pure functions with no side effects, so they can be nested freely and
are safe to call over RPC.

Every argument is converted to its string representation before use, which means
numbers can be passed where text is expected.
Commands that count characters, such as `string.length` and `string.substr`, count
utf-8 characters rather than bytes.

## string.length

    # string.length = «text»

    string.length = "héllo"    # 5

Returns the number of utf-8 characters in the text.

## string.equals

    # string.equals = «text», «other»[, ...]

    string.equals = (d.name), "first.iso", "second.iso"

Returns `1` if the first argument equals any of the following arguments,
otherwise `0`.

## string.starts_with, string.ends_with

    # string.starts_with = «text», «prefix»[, ...]
    # string.ends_with = «text», «tail»[, ...]

    string.starts_with = (t.url), "http://", "https://"
    string.ends_with = (d.name), ".iso"

Returns `1` if the text begins, or ends, with any of the given prefixes or
tails.

## string.contains, string.contains_i

    # string.contains = «haystack», «needle»[, ...]
    # string.contains_i = «haystack», «needle»[, ...]

    string.contains = (t.url), "retracker.local"
    string.contains_i = (t.url), "RETRACKER.local"

Returns `1` if the haystack contains any of the needles.
The `_i` variant compares case-insensitively, and only handles ascii.

## string.substr

    # string.substr = «text»[, «position»[, «count»[, «default»]]]

    string.substr = "abcdef", 2, 3       # "cde"
    string.substr = "abcdef", -2         # "ef"
    string.substr = "abcdef", 10, 1, "?" # "?"

Extracts a part of the text, starting at `position` and spanning `count`
characters.
The position defaults to the start of the text, and the count to the rest of it.
A negative position is relative to the end of the text.
If the position falls outside the text, the default value is returned instead,
which is the empty string unless given.

## string.split

    # string.split = «text», «delimiter»

    string.split = "a.b.c", "."    # {"a", "b", "c"}
    string.split = "abc", ""       # {"a", "b", "c"}

Splits the text into a list, keeping empty fields.
An empty delimiter splits the text into its utf-8 characters.

## string.join

    # string.join = «delimiter»[, «object»[, ...]]

    string.join = "-", (string.split, "a.b.c", ".")    # "a-b-c"

Concatenates the objects, inserting the delimiter between them.
Lists are flattened, so the result of `string.split` can be joined back
together.

## string.lpad, string.rpad

    # string.lpad = «text», «length»[, «padding»]
    # string.rpad = «text», «length»[, «padding»]

    string.lpad = 7, 3, 0    # "007"
    string.rpad = "a", 3     # "a  "

Pads the text at the start, or the end, until it is `length` characters long.
The padding defaults to a single space and is repeated as needed.
Text that is already long enough is returned unchanged.

## string.strip, string.lstrip, string.rstrip

    # string.strip = «text»[, «strippable»[, ...]]
    # string.lstrip = «text»[, «head»[, ...]]
    # string.rstrip = «text»[, «tail»[, ...]]

    string.strip = "  padded  "      # "padded"
    string.strip = "//path//", "/"   # "path"

Removes characters from both ends of the text, or from only the start or the
end.
The arguments after the text form a set of utf-8 characters to remove.
Without them, whitespace is removed.

## string.map

    # string.map = «text», {«old», «new»}[, ...]

    string.map = (d.state), {0, "stopped"}, {1, "started"}

Returns the replacement of the first pair whose `old` value equals the whole
text.
If no pair matches, the text is returned unchanged.

## string.replace

    # string.replace = «text», {«old», «new»}[, ...]

    string.replace = "a-b-c", {"-", "+"}    # "a+b+c"

Replaces every occurrence of `old` with `new`.
The pairs are applied in order, so a later pair operates on the result of the
earlier ones.

## Example: dropping unwanted trackers

The following disables every tracker that points at `retracker.local` as soon as
a download is inserted.

    method.set_key = event.download.inserted, drop_retracker, \
      ((t.multicall, default, "branch=(string.contains,(t.url),retracker.local),((t.disable))"))
