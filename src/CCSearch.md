# Introduction

CCSearch stands for "Constrained Collection Search".  Although originating from a component of a Fantasy Sports system, it is has been abstracted into a system of general utility.  CCSearch performs an optimized high-speed state search for maximum-value collections of items which satisfy certain constraints.  This is accomplished through aggressive pruning.  The algorithm is optimal for knapsack-like problems with the following properties:

* There exists a set of items from which the user wishes to form collections of a fixed size.  Each item has a known cost and a known value.  The goal is to find those collections which maximize value while keeping the cost under some specified limit.  For example we are given set of Fantasy Sport players,  each with an associated salary (set by the tournament host) and value (predicted performance, based on the user's model).  A fantasy team may consist of 10 players with a combined salary no more than 50K.  

* A set of "features" of the items.  A feature consists of a set of groups (values the feature may take) and an assignment of every item to one or more of those groups.  Some features may be partitions (each item is in one and only one group), while others may not.  Examples of features could be the player's field position, which games the player appears in, the real-life team they play for, and whether they are a hitter or pitcher.

* A choice of "primary feature."  We are required to choose a (known) fixed number of items from each group of the primary feature (summing to the total number in the collection).  For example, in fantasy baseball the field position would be the primary feature.  Each fantasy team has a specified number of pitchers, outfielders, etc, which together form the 10 player lineup.

* A set of "constraints" for collections (other than the overall cost cap).  These typically are of the form "no more than n items from any one group of a certain feature" or "at least one item in each of n groups of a certain feature."  However, general feature-based user-defined constraints also may be accommodated.  Examples of common constraints are "no more than 4 players from the same team" and "at least one player from each of 2 games." 

* We know or hope or suspect that there is a thick band of high-value collections culminating in the absolute maximum.  I.e., there isn't a single best collection head-and-shoulders above the rest.

* We don't care about finding the absolute maximum solution or every solution within the band, but rather the vast majority --- or even just a large number of them.  There are many reasons this could be the case:

	* The values we assign have error bars, and rigidly seeking the absolute maximum is pointless and possibly detrimental

	* We don't need the absolute maximum, just a very good solution

	* We wish to diversify our risk by picking some subset of solutions from this band.  We may want a minimally correlated subset, as determined by additional analysis outside the scope of the search itself. For example if we intend to enter 20 different fantasy teams in a tournament, we may wish to be presented with a few thousand from which we subsequently select 20 which diversify our risk. 

Any system which meets these criteria could benefit from the use of CCSearch.  

This said, the CCSearch project grew out of a Fantasy Baseball project and can efficiently search the state space for almost any fantasy sport (once some separate model has assigned predictive values), to locate a set of fantasy team candidates.  

As a second simple example, consider a collection of playing cards whose costs are randomly assigned, whose values are their standard numeric face values, and whose features are suite and whether royal or not.  If there are multiple decks, another feature could be the choice of deck.  If we simply wish to find the very best 5-card hand subject to some maximum cost constraint, the standard knapsack algorithm would be useful.  But suppose we have additional constraints: at most 3 cards can come from the same suite and at least 2 cards must be from different decks.  If we don't really care about finding the absolute best hand, but rather a large set of hands pretty close to it, then our algorithm will be of use.  

For details of the CCSearch Algorithm itself, please see doc/SearchAlgo.*

# Use of the system.  

There are 4 ways to use this reference implementation:

* Via the apitest.py front-end.  This is the easiest way, and requires just compilation of the library (i.e. running "make").  It can handle delimited-text input data files and general configurations.  However, it is confined to using the two classes of intrinsic constraints we support.  User-derived constraint classes aren't accessible (at this time) via apitest.py.

* Write your own Python script which munges data to your liking and calls our C++ backend when needed.  For this, you'll need to compile the backend (i.e. run "make") and include the line 'exec(open("ccsapi.py").read())' in your Python 3 code.  Yes, it's clumsy but it works.  See apitest.py for sample code to use as a template.  As with apitest.py, user-derived constraint classes are not supported at this time.

* Write code which links to the C++ library or C++ source code directly.  This approach allows user-derived constraint classes (which can be added directly via the appropriate call after instantiation).  

* Write you own better implementation.  By all means, use this as a template or example and write a better, faster, and quicker implementation.  I'm told that in Haskell it will take 1 line and consisting of 12 unicode characters.  You just need to spend a few years figuring out which 12 characters (or run an exhaustive search using a different algorithm).

# Notes on the Reference implementation

The Python 3 code is functional but not elegant (I'm not a Python programmer).  The C++ code should work, and tries to be threadsafe.  However, this is a reference implementation not a battle-hardened production one.  I tried to give it a passable design and did some basic debugging.  However, it has no unit tests and there is no regression/coverage testing suite.  Nor did a great deal of effort go into the overall design.  It is simple and functional, with an eye to quick production rather than rigorous testing.  If there is sufficient interest, I may put some effort toward the latter in the near future.  

# C++ Source Code layout

* OMutex.h:		Defines a convenient RAII Mutex template implementation.  Standalone.

* OGlobal.h:		Defines some global functions (static member fns of OGlobal) for bad-value management.  Standalone.

* OFeature.h/.cpp:	Defines the concept of feature (OFeature), a table representing the membership function for items in groups.  It also allows the culling of items (and appropriate updates to the feature) as well as testing whether a feature is a partition, etc.  Depends only on OMutex, so effectively standalone. 

* OColl.h/.cpp:		Defines a simple memory manager (OCollMM) for large arrays of collections.  This is used by the search algorithm to store solutions as they arise.  OCollMM has GC facilities for purging obsolete solutions (ex. when a better top solution is found, existing solutions may fall below the new threshold for acceptance).  Depends only on OMutex and OGlobal, so effectively standalone.

* OCFN.h/.cpp:		Defines the concept of constraint (OCFN) as a hierarchy of ABCs, allowing user-defined class derivation (though at this point only via C++ linkage and not the python API), as well as 2 built-in generic constraints (which together cover all the common fantasy sport constraints).  Depends on OConfig (a parent OConfig must be provided to give access to the features needed by the constraints when configured). 

* OConfig.h/.cpp:	Gathers all the config info, features, constraints, the collection MM, etc into a single structure.  It also hosts most of the major functions called elsewhere.  Depends on OFeature, OFCN, OColl, OMutex, OGlobal.  

* OSearch.h/.cpp:	The search algorithm itself.  This consists of various record classes for stuff precomputed prior to search (sorting within groups, by groups, etc), as well as the OSearch class which performs the search.  It depends on everything.  

* OAPI.h/.cpp:		A useful set of API routines.  These are wrapped in OPython for export to Python, but can just as easily serve as a C++ API.  Only a couple have any meat.  Depends on everything. 

* OPython.h/.cpp:	No meat.  Literally exports a bunch of plain-ol' wrappers for the functions in OAPI, along with a global instance of OConfig (as needed by python).  Depends on everything.

