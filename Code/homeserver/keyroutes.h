#ifndef KEYROUTES_H
#define KEYROUTES_H

class QHttpServer;
class Database;

/**
 * @brief Register POST /user/keys (publish own keys) and GET /user/keys (fetch a recipient's bundle).
 * @param server HTTP server to add the routes to.
 * @param database Open database; must outlive the server.
 * @note Request and response formats are documented in DATABASE.md § Public key API.
 */
void addKeyRoutes(QHttpServer &server, Database &database);

#endif
