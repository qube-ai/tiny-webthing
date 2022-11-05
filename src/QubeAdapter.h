#include "TinyAdapter.h"

/*
 * Wrapper for the TinyAdapter class.
 * Just to comply with the QubeAdapter interface.
 */
class QubeAdapter : public TinyAdapter
{
public:
    QubeAdapter(String _websocketUrl, int _port, String _websocketPath) : TinyAdapter(_websocketUrl, _port, _websocketPath) {}
};
