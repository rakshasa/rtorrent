# The ui.* Command Group

Commands in this group control aspects of the ‘curses’ UI.

## ui.current_view (since 0.9.7)

Returns the name of the currently selected view. 
Typical uses are to change and then restore the active view, 
or rotate through a set of views
(which requires querying the view, to find the next one).

## ui.current_view.set=NAME

Change the view the user sees. `view.list` gives you a list of all the open views.

## ui.unfocus_download

Used internally to bump the selection away from an item about to be erased.