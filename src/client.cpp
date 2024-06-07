#include "client.h"
#include "server.h"

Client::Client(std::string id, const Server &server) : server(&server), id(id)
{
    crypto::generate_key(public_key, private_key);
}

std::string Client::get_id()
{
    return id;
}

double Client::get_wallet() const
{
    return server->get_wallet(id);
}

std::string Client::get_publickey() const
{
    return public_key;
}

std::string Client::sign(std::string txt) const
{
    return crypto::signMessage(private_key, txt);
}

bool Client::transfer_money(std::string receiver, double value) const
{
    std::string trx = id + "-" + receiver + "-" + std::to_string(value);
    std::string signature = crypto::signMessage(private_key, trx);
    return server->add_pending_trx(trx, signature);
}

size_t Client::generate_nonce() const
{
    static std::random_device rd;
    static std::mt19937_64 engine(rd());
    static std::uniform_int_distribution<size_t> distribution(0, std::numeric_limits<size_t>::max());

    return distribution(engine);
}