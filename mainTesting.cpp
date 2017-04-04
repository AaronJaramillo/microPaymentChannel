#include <bitcoin/bitcoin.hpp>
#include <bitcoin/client.hpp>
#include "payer.cpp"

#include <string.h>

using namespace bc;
using namespace bc::wallet;
using namespace bc::machine;
using namespace bc::chain;

int main()
{
	HD_Wallet wallet(split(""));
	ec_public payee = wallet.childPublicKey(2).point();

	channelPayer channel(wallet, 1, payee, "1.98");
	std::cout << encode_base16(channel.getBond().to_data()) << std::endl;
	channel.signBond();
	std::cout << encode_base16(channel.getBond().to_data()) << std::endl;
	std::cout << encode_base16(channel.getRefund().to_data()) << std::endl;

	channel.signRefund();
	std::cout << "\n" << encode_base16(channel.getRefund().to_data()) << std::endl;
	std::cout << script().verify(channel.getBond(), 0, all_rules).message() << std::endl;
	channel.validRefund(channel.getRefund());



}