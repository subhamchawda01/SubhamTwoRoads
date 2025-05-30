
/// BMF:
/// Get Order CancelReplace to be working.
/// UMDF is working properly
/// but the field is_intermediate_ needs to be set properly
/// the field agg_side_ is not available

/// General:
/// If Settings file ( ors.fg ) has CancelOnDisconnect=1 then
/// on either client disconnect or the ORS crashing cancel all active orders
/// as long as we are connected and logged in

/// TODO_OPT all locks ... make them read-write locks and approproiately use them
