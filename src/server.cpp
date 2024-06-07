#include "server.h"
#include "client.h"

std::vector<std::string> pending_trxs;

void show_wallets(const Server &server)
{
    std::cout << std::string(20, '*') << std::endl;
    for (const auto &client : server.clients)
        std::cout << client.first->get_id() << " : " << client.second << std::endl;
    std::cout << std::string(20, '*') << std::endl;
}

/** append a random four digit number to a string  */
std::string append_random_four_digit_number(const std::string &str)
{
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> distr(0, 9);
    std::string result{str};

    result += std::to_string(distr(gen));
    result += std::to_string(distr(gen));
    result += std::to_string(distr(gen));
    result += std::to_string(distr(gen));
    return result;
}

Server::Server()
{
}

std::shared_ptr<Client> Server::add_client(const std::string id)
{
    std::string actual_id = id;
    if (get_client(id) != nullptr)
        actual_id = append_random_four_digit_number(id);
    std::shared_ptr<Client> client = std::make_shared<Client>(actual_id, *this);
    clients[client] = 5.0;
    return client;
}

std::shared_ptr<Client> Server::get_client(const std::string id) const
{
    for (const auto &pair : clients)
        if (pair.first->get_id() == id)
            return pair.first;
    return nullptr;
}

double Server::get_wallet(const std::string id) const
{
    std::shared_ptr<Client> client = get_client(id);
    if (client != nullptr)
        return clients.at(client);
    return 0;
}

bool Server::parse_trx(std::string trx, std::string &sender, std::string &receiver, double &value)
{
    std::string s, r;
    double v;
    char delimiter = '-';

    std::istringstream trx_stream(trx);
    std::getline(trx_stream, s, delimiter);
    std::getline(trx_stream, r, delimiter);
    trx_stream >> v;
    if (!trx_stream.eof() || trx_stream.fail())
        throw std::runtime_error("invalid trx");

    sender = s;
    receiver = r;
    value = v;
    return true; // ? the reture value seems not very useful.
}

bool Server::add_pending_trx(std::string trx, std::string signature) const
{
    // verify the trx
    std::string sender{}, receiver{};
    double value;
    try
    {
        parse_trx(trx, sender, receiver, value);
    }
    catch (const std::runtime_error &e)
    {
        return false;
    }
    if (value <= 0)
        return false;
    auto sender_client = get_client(sender);
    auto receiver_client = get_client(receiver);
    if (!sender_client || !receiver_client || get_wallet(sender) < value)
        return false;

    // verify the signature
    if (!crypto::verifySignature(sender_client->get_publickey(), trx, signature))
        return false;

    pending_trxs.push_back(trx);
    return true;
}

size_t Server::mine()
{

    std::string mempool{};
    for (const auto &trx : pending_trxs)
        mempool += trx;

    size_t nonce;
    while (true)
        for (auto &client : clients)
        {
            nonce = client.first->generate_nonce();
            std::string hash = crypto::sha256(mempool + std::to_string(nonce));
            if (hash.substr(0, 10).find("000") != std::string::npos)
            {
                client.second += 6.25;
                // Finish the transactions and remove succeed transaction in the pending_trx
                // ? How to handle the failed trx?
                pending_trxs.erase(
                    std::remove_if(
                        pending_trxs.begin(), pending_trxs.end(),
                        [this](auto trx)
                        {
                            std::string sender{}, receiver{};
                            double value;
                            parse_trx(trx, sender, receiver, value);
                            auto sender_client = get_client(sender);
                            if (sender_client->get_wallet() < value)
                                return false;
                            auto receiver_client = get_client(receiver);
                            this->clients[sender_client] -= value;
                            this->clients[receiver_client] += value;
                            return true;
                        }),
                    pending_trxs.end());
                return nonce;
            }
        }
}