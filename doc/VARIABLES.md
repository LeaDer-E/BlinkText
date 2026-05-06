# BlinkText Variables Guide

This document explains the full Variables system supported by BlinkText.

It is intended to work as:
- a user guide
- a GitHub documentation page
- a future Wiki page
- a migration reference for Beeftext users

## Overview

BlinkText variables allow a snippet to do more than insert fixed text.

With variables, a snippet can:
- insert clipboard content
- insert previous clipboard content
- insert the current date or time
- ask the user for text input
- send a key press
- send a shortcut
- wait for a short delay
- place the caret at a specific final position
- reuse another snippet
- transform another snippet to trimmed, upper, or lower case
- read a Windows environment variable
- run a local PowerShell script and insert its output

BlinkText resolves variables right before expansion.

## Variable Syntax

BlinkText variables use one of these forms:

```text
#{variableName}
#{variableName:parameter}
#{variableName:parameter:timeoutMs}
```

Examples:

```text
#{clipboard}
#{dateTime:yyyy-MM-dd}
#{shortcut:Ctrl+Shift+S}
#{powershell:C:\Temp\script.ps1:5000}
```

## Core Rules

- Variable names are case-sensitive.
- Unknown variables fail safely.
- Malformed variables must not crash BlinkText.
- Variables are resolved right before expansion.
- Action order is preserved exactly.
- Action variables do not appear in the final output text.
- BlinkText stays fully offline.

## Escaping Rules

BlinkText supports these escapes inside snippet text:

```text
\}   -> literal }
\\   -> literal backslash
```

Examples:

```text
This is a literal brace: \}
This is a literal slash: \\
```

### Important combo parsing note

For combo-like variables:

```text
#{combo:trigger}
#{trim:trigger}
#{upper:trigger}
#{lower:trigger}
```

the trigger parameter is treated as a raw string until the closing `}`.

That means a single backslash is valid.

Example:

```text
#{combo:t1t\}
```

This resolves the trigger:

```text
t1t\
```

You do not need to write `\\` for combo trigger names.

## Visible Variables vs Action Variables

### Visible variables

These contribute visible text to the final output:

- `#{clipboard}`
- `#{previousClipboard}`
- `#{date}`
- `#{time}`
- `#{dateTime}`
- `#{dateTime:...}`
- `#{input:...}`
- `#{combo:...}`
- `#{trim:...}`
- `#{upper:...}`
- `#{lower:...}`
- `#{envVar:...}`
- `#{powershell:...}`

### Action variables

These do not appear as text in the final output:

- `#{key:...}`
- `#{shortcut:...}`
- `#{delay:...}`
- `#{cursor}`

## Expansion Order

BlinkText preserves the exact order between text and actions.

Example:

```text
Hello #{input:}#{key:Tab}#{clipboard}
```

Execution order:
1. Ask for the input value
2. Insert `Hello ` plus the entered value
3. Send `Tab`
4. Insert the clipboard content

This order is preserved both in the Test Area and in external applications.

## Beeftext Compatibility

BlinkText is designed to be friendly to users migrating from Beeftext, but it is not a byte-for-byte clone of Beeftext behavior in every edge case.

### Fully compatible or very close

These variables are supported in a Beeftext-like way and are generally safe to migrate:

- `#{clipboard}`
- `#{date}`
- `#{time}`
- `#{dateTime}`
- `#{dateTime:...}`
- `#{combo:...}`
- `#{upper:...}`
- `#{lower:...}`
- `#{cursor}`
- `#{input:...}`
- `#{envVar:...}`
- `#{powershell:...}`

### BlinkText extensions

These features are BlinkText-specific additions and are not standard Beeftext variables:

- `#{previousClipboard}`
- `#{key:...}`
- `#{shortcut:...}`
- `#{delay:...}`
- `#{trim:...}`
- built-in clipboard triggers:
  - `\\`
  - `//`

### Important differences from Beeftext

- BlinkText focuses on the main `#{dateTime:format}` behavior.
- Advanced Beeftext date shifting syntax such as:

```text
#{dateTime:+1d:yyyy-MM-dd}
```

is not documented as fully compatible in BlinkText.

- Escaping behavior is close to Beeftext, but not guaranteed to be identical in every parser edge case.

- BlinkText `#{input:...}` is more advanced than Beeftext:
  - multi-line input popup
  - `Shift+Enter` inserts a new line
  - `Enter` confirms input
  - `Cancel` aborts the entire expansion safely

- BlinkText keeps clipboard variable values stable during one expansion using a fixed clipboard snapshot.

## Clipboard Variables

### `#{clipboard}`

Inserts the clipboard text captured at the start of the expansion.

Example:

```text
Before
#{clipboard}
After
```

Important behavior:
- BlinkText captures the clipboard once when the expansion starts.
- During that one expansion, `#{clipboard}` always uses that snapshot.
- Internal clipboard changes used by BlinkText do not change the value of `#{clipboard}` during the same expansion.
- Long clipboard content is pasted efficiently instead of being typed character by character.

### `#{previousClipboard}`

Inserts the stable clipboard snapshot used during the current expansion.

Example:

```text
Prev:[#{previousClipboard}]
```

Important behavior:
- During one expansion, BlinkText uses a stable clipboard snapshot.
- Internal clipboard operations must not overwrite the value used by `#{previousClipboard}`.
- The clipboard is restored after the whole expansion finishes or aborts.

### Related built-in clipboard triggers

BlinkText also supports two optional built-in clipboard helper triggers:

- `\\`
- `//`

These are not variables. They are configurable engine features in `Engine Settings`.

## Date and Time Variables

### `#{date}`

Inserts the current date.

Example:

```text
#{date}
```

### `#{time}`

Inserts the current time.

Example:

```text
#{time}
```

### `#{dateTime}`

Inserts the current date and time.

Example:

```text
#{dateTime}
```

### `#{dateTime:customFormat}`

Inserts the current date and time using a custom format string.

Examples:

```text
#{dateTime:yyyy-MM-dd}
#{dateTime:HH:mm:ss}
#{dateTime:yyyy-MM-dd HH:mm:ss}
```

Use this when you want a specific output format.

## Input Variable

### `#{input:}`

Example:

```text
Name: #{input:}
```

Behavior:
- BlinkText shows an input popup during expansion.
- The user types a value.
- That value is inserted exactly where the variable appears.

### Multi-line input behavior

- `Shift+Enter` inserts a new line inside the input box
- `Enter` confirms the dialog

### Cancel behavior

If the user:
- clicks `Cancel`
- closes the popup
- presses `Esc`

then BlinkText:
- aborts the entire expansion immediately
- does not continue remaining text
- does not continue remaining keys
- does not continue remaining delays
- restores the clipboard safely
- does not crash

Example:

```text
Start
#{input:}
End
```

If the user cancels the popup:
- `End` is not inserted
- the expansion stops completely

## Key Variable

### `#{key:keyName}`

Examples:

```text
#{key:Tab}
#{key:Enter}
#{key:Escape}
#{key:Home}
#{key:End}
#{key:Left}
#{key:Right}
```

Key names are case-insensitive.

All of these should behave the same:

```text
#{key:home}
#{key:Home}
#{key:HOME}
#{key:HoMe}
```

### Supported key names

```text
space
tab
enter
insert
delete
home
end
pageUp
pageDown
up
down
left
right
escape
printScreen
pause
numLock
volumeMute
volumeUp
volumeDown
mediaNextTrack
mediaPreviousTrack
mediaStop
mediaPlayPause
mediaSelect
windows
control
alt
shift
f1
f2
f3
f4
f5
f6
f7
f8
f9
f10
f11
f12
f13
f14
f15
f16
f17
f18
f19
f20
f21
f22
f23
f24
```

### Supported aliases

```text
esc           -> escape
pgup          -> pageUp
pgdn          -> pageDown
pageup        -> pageUp
pagedown      -> pageDown
win           -> windows
ctrl          -> control
del           -> delete
ins           -> insert
prtsc         -> printScreen
printscreen   -> printScreen
```

### Important `#{key:Enter}` note

`#{key:Enter}` sends a real Enter key press.

It does not behave like plain text.

Example:

```text
Line1#{key:Enter}Line2
```

Expected result:

```text
Line1
Line2
```

Now compare that to:

```text
Line1
#{key:Enter}
Line2
```

This already contains real line breaks in the snippet text, and then it also sends an actual Enter key.

In apps like Notepad or Word, this often means an extra blank line appears between the two visible lines.

So:
- if you want plain text line breaks, write them as plain text
- if you want a real Enter key action, use `#{key:Enter}`

## Shortcut Variable

### `#{shortcut:Shortcut}`

Examples:

```text
#{shortcut:Ctrl+C}
#{shortcut:Ctrl+V}
#{shortcut:Ctrl+A}
#{shortcut:Ctrl+Shift+S}
#{shortcut:Alt+F4}
#{shortcut:Win+R}
```

Behavior:
- Sends the shortcut dynamically
- Parses modifiers automatically
- Keeps execution order with other variables

Supported modifiers:
- `Ctrl`
- `Shift`
- `Alt`
- `Win`

## Delay Variable

### `#{delay:milliseconds}`

Examples:

```text
#{delay:1000}
#{delay:50}
```

Meaning:
- `1000` = 1 second
- `50` = 50 milliseconds

Use it to insert a controlled delay between actions.

Example:

```text
Hello#{delay:500}World
```

## Cursor Variable

### `#{cursor}`

Example:

```text
Hello #{cursor}World
```

Expected final result:

```text
Hello |World
```

The marker itself is removed from the final output.

BlinkText calculates the final cursor position using the final visible output only.

That means it counts:
- normal text
- clipboard text
- previous clipboard text
- input text
- combo results
- environment variable values
- PowerShell output
- actual line breaks that appear in the final visible output

And it does not count:
- `#{delay:...}`
- `#{key:...}`
- `#{shortcut:...}`
- `#{cursor}`

## Combo Variables

These variables reuse another snippet by trigger.

### `#{combo:trigger}`

Inserts another snippet exactly as resolved.

### `#{trim:trigger}`

Inserts another snippet after trimming leading and trailing whitespace only.

Note:
- `#{trim:...}` is a BlinkText extension.
- It is useful when the source snippet contains unwanted spaces or blank lines at the start or end.

### `#{upper:trigger}`

Inserts another snippet converted to uppercase.

### `#{lower:trigger}`

Inserts another snippet converted to lowercase.

Examples:

```text
#{combo:sig\}
#{trim:summary\}
#{upper:name\}
#{lower:TITLE\}
```

### Combo recursion protection

BlinkText prevents infinite loops.

Rules:
- maximum recursion depth = `5`
- recursion fails safely
- unknown triggers do not crash the app

### Arabic and non-Latin text

`#{upper:...}` and `#{lower:...}` now use Unicode-aware Windows casing instead of simple per-character fallback only.

That means:
- Latin text is converted more reliably
- Arabic text is not broken
- scripts that do not have uppercase/lowercase forms, such as Arabic, remain unchanged

Example:

```text
Hello عالم
```

Expected casing behavior:
- the Latin part can change case
- the Arabic part remains intact

So:
- `#{upper:...}` behaves similarly to Beeftext, but safely for mixed text
- `#{lower:...}` behaves similarly to Beeftext, but safely for mixed text

## Environment Variable

### `#{envVar:VARIABLE_NAME}`

Examples:

```text
#{envVar:USERNAME}
#{envVar:PATH}
```

Behavior:
- reads the matching Windows environment variable
- inserts its value if found
- fails safely if not found

## PowerShell Variable

### `#{powershell:path}`

### `#{powershell:path:timeoutMs}`

Examples:

```text
#{powershell:C:\Temp\script.ps1}
#{powershell:C:\Temp\script.ps1:5000}
#{powershell:C:\Temp\script.ps1:0}
```

Behavior:
- runs a local `.ps1` file explicitly chosen by the user
- captures `stdout` only
- inserts the output into the snippet
- does not show a console window

Timeout rules:
- default timeout = `10000 ms`
- `0` = wait indefinitely
- positive value = stop after that number of milliseconds

Safety rules:
- only local `.ps1` files are supported
- no inline PowerShell execution
- no automatic downloads
- no remote execution

## Mixed Examples

### Example 1

```text
Hello #{input:}#{key:Tab}#{clipboard}
```

Behavior:
1. Ask for input
2. Insert `Hello ` plus the entered value
3. Send `Tab`
4. Insert clipboard content

### Example 2

```text
Start
#{clipboard}
#{delay:50}
Done
```

Behavior:
1. Insert `Start`
2. Insert clipboard content
3. Wait `50 ms`
4. Insert `Done`

### Example 3

```text
12345#{key:Left}#{key:Left}X#{key:Right}Y#{key:Home}A#{key:End}Z
```

Expected result:

```text
A123X4Y5Z
```

### Example 4

```text
#{clipboard}
Prev:[#{previousClipboard}]
```

Behavior:
- both values are resolved from the clipboard state captured at the start of that expansion

## Performance Notes

BlinkText is designed to stay fast, but variable-heavy snippets are naturally slower than plain text snippets.

General guidance:
- plain text snippets are the fastest
- `#{clipboard}` is faster than typing large copied text manually
- `#{key:...}` and `#{shortcut:...}` use ordered actions and may be slightly slower for stability
- rich text editors such as Word may need safer timing than plain editors like Notepad

## Privacy Notes

BlinkText is fully local.

It does not:
- collect user data
- send snippet content anywhere
- upload clipboard data
- store input dialog content outside the local expansion flow

Clipboard and input values are used locally only for the current expansion.

## Troubleshooting

### I see extra blank lines

Check whether the snippet contains:
- real text line breaks
- plus `#{key:Enter}`

If both are used together, the extra spacing may be expected.

### `#{cursor}` is not where I expected

Check whether:
- the snippet contains many action variables before the cursor marker
- the target application handles caret movement differently

Start by testing in:
- BlinkText Test Area
- Notepad
- then Word

### `#{input:}` stops everything

This is expected if the popup is cancelled.

BlinkText treats input cancellation as an aborted expansion for safety.

## Quick Reference

```text
#{clipboard}
#{previousClipboard}
#{date}
#{time}
#{dateTime}
#{dateTime:yyyy-MM-dd HH:mm:ss}
#{input:}
#{key:Tab}
#{key:Enter}
#{shortcut:Ctrl+Shift+S}
#{delay:1000}
#{cursor}
#{combo:trigger}
#{trim:trigger}
#{upper:trigger}
#{lower:trigger}
#{envVar:USERNAME}
#{powershell:C:\Temp\script.ps1}
#{powershell:C:\Temp\script.ps1:5000}
#{powershell:C:\Temp\script.ps1:0}
```

## Suggested Wiki Title

If you later move this file into a GitHub Wiki, a good page title would be:

`BlinkText Variables Guide`
