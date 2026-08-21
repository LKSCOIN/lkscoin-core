// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Copyright (c) 2014-2023 The Lksc Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>

#include <chainparamsseeds.h>
#include <consensus/merkle.h>
#include <llmq/params.h>
#include <tinyformat.h>
#include <util/ranges.h>
#include <util/system.h>
#include <versionbitsinfo.h>

#include <arith_uint256.h>

#include <assert.h>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

static CBlock CreateGenesisBlock(const char* pszTimestamp, const CScript& genesisOutputScript, uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
{
    CMutableTransaction txNew;
    txNew.nVersion = 1;
    txNew.vin.resize(1);
    txNew.vout.resize(1);
    txNew.vin[0].scriptSig = CScript() << 486604799 << CScriptNum(4) << std::vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
    txNew.vout[0].nValue = genesisReward;
    txNew.vout[0].scriptPubKey = genesisOutputScript;

    CBlock genesis;
    genesis.nTime    = nTime;
    genesis.nBits    = nBits;
    genesis.nNonce   = nNonce;
    genesis.nVersion = nVersion;
    genesis.vtx.push_back(MakeTransactionRef(std::move(txNew)));
    genesis.hashPrevBlock.SetNull();
    genesis.hashMerkleRoot = BlockMerkleRoot(genesis);
    return genesis;
}

static CBlock CreateDevNetGenesisBlock(const uint256 &prevBlockHash, const std::string& devNetName, uint32_t nTime, uint32_t nNonce, uint32_t nBits, const CAmount& genesisReward)
{
    assert(!devNetName.empty());

    CMutableTransaction txNew;
    txNew.nVersion = 1;
    txNew.vin.resize(1);
    txNew.vout.resize(1);
    // put height (BIP34) and devnet name into coinbase
    txNew.vin[0].scriptSig = CScript() << 1 << std::vector<unsigned char>(devNetName.begin(), devNetName.end());
    txNew.vout[0].nValue = genesisReward;
    txNew.vout[0].scriptPubKey = CScript() << OP_RETURN;

    CBlock genesis;
    genesis.nTime    = nTime;
    genesis.nBits    = nBits;
    genesis.nNonce   = nNonce;
    genesis.nVersion = 4;
    genesis.vtx.push_back(MakeTransactionRef(std::move(txNew)));
    genesis.hashPrevBlock = prevBlockHash;
    genesis.hashMerkleRoot = BlockMerkleRoot(genesis);
    return genesis;
}

/**
 * Build the genesis block. Note that the output of its generation
 * transaction cannot be spent since it did not originally exist in the
 * database.
 *
 * CBlock(hash=00000ffd590b14, ver=1, hashPrevBlock=00000000000000, hashMerkleRoot=e0028e, nTime=1390095618, nBits=1e0ffff0, nNonce=28917698, vtx=1)
 *   CTransaction(hash=e0028e, ver=1, vin.size=1, vout.size=1, nLockTime=0)
 *     CTxIn(COutPoint(000000, -1), coinbase 04ffff001d01044c5957697265642030392f4a616e2f3230313420546865204772616e64204578706572696d656e7420476f6573204c6976653a204f76657273746f636b2e636f6d204973204e6f7720416363657074696e6720426974636f696e73)
 *     CTxOut(nValue=50.00000000, scriptPubKey=0xA9037BAC7050C479B121CF)
 *   vMerkleTree: e0028e
 */
static CBlock CreateGenesisBlock(uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
{
    const char* pszTimestamp = "Creiamo un mondo in cui tutti possano guadagnare";
    const CScript genesisOutputScript = CScript() << ParseHex("04dfd4813aecbe042ef1c348b539ad0d64116476f784d4110598874537cb52e29dd18e76ba66e932a0a377a01371c69d91f6a363a819bcc3c75a35c1dadbf22738") << OP_CHECKSIG;
    return CreateGenesisBlock(pszTimestamp, genesisOutputScript, nTime, nNonce, nBits, nVersion, genesisReward);
}

static CBlock FindDevNetGenesisBlock(const CBlock &prevBlock, const CAmount& reward)
{
    std::string devNetName = gArgs.GetDevNetName();
    assert(!devNetName.empty());

    CBlock block = CreateDevNetGenesisBlock(prevBlock.GetHash(), devNetName, prevBlock.nTime + 1, 0, prevBlock.nBits, reward);

    arith_uint256 bnTarget;
    bnTarget.SetCompact(block.nBits);

    for (uint32_t nNonce = 0; nNonce < UINT32_MAX; nNonce++) {
        block.nNonce = nNonce;

        uint256 hash = block.GetHash();
        if (UintToArith256(hash) <= bnTarget)
            return block;
    }

    // This is very unlikely to happen as we start the devnet with a very low difficulty. In many cases even the first
    // iteration of the above loop will give a result already
    error("FindDevNetGenesisBlock: could not find devnet genesis block for %s", devNetName);
    assert(false);
}

void CChainParams::AddLLMQ(Consensus::LLMQType llmqType)
{
    assert(!GetLLMQ(llmqType).has_value());
    for (const auto& llmq_param : Consensus::available_llmqs) {
        if (llmq_param.type == llmqType) {
            consensus.llmqs.push_back(llmq_param);
            return;
        }
    }
    error("CChainParams::%s: unknown LLMQ type %d", __func__, static_cast<uint8_t>(llmqType));
    assert(false);
}

std::optional<Consensus::LLMQParams> CChainParams::GetLLMQ(Consensus::LLMQType llmqType) const
{
    for (const auto& llmq_param : consensus.llmqs) {
        if (llmq_param.type == llmqType) {
            return std::make_optional(llmq_param);
        }
    }
    return std::nullopt;
}

/**
 * Main network
 */
class CMainParams : public CChainParams {
public:
    CMainParams() {
        strNetworkID = CBaseChainParams::MAIN;
        consensus.nSubsidyHalvingInterval = 210240; // Note: actual number of blocks per calendar year with DGW v3 is ~200700 (for example 449750 - 249050)
        consensus.nMasternodePaymentsStartBlock = 20000;  // not true, but it's ok as long as it's less then nMasternodePaymentsIncreaseBlock
        consensus.nMasternodePaymentsIncreaseBlock = 108000;  // actual historical value
        consensus.nMasternodePaymentsIncreasePeriod = 576*30; // 17280 - actual historical value
        consensus.nInstantSendConfirmationsRequired = 6;
        consensus.nInstantSendKeepLock = 24;
        consensus.nBudgetPaymentsStartBlock = 328008; // actual historical value
        consensus.nBudgetPaymentsCycleBlocks = 16616; // ~(60*24*30)/2.6, actual number of blocks per month is 200700 / 12 = 16725
        consensus.nBudgetPaymentsWindowBlocks = 100;
        consensus.nSuperblockStartBlock = 614820; // The block at which 12.1 goes live (end of final 12.0 budget cycle)
        consensus.nSuperblockStartHash = uint256();
        consensus.nSuperblockCycle = 16616; // ~(60*24*30)/2.6, actual number of blocks per month is 200700 / 12 = 16725
        consensus.nSuperblockMaturityWindow = 1662; // ~(60*24*3)/2.6, ~3 days before actual Superblock is emitted
        consensus.nGovernanceMinQuorum = 10;
        consensus.nGovernanceFilterElements = 20000;
        consensus.nMasternodeMinimumConfirmations = 15;
        consensus.BIP34Height = 951;
        consensus.BIP34Hash = uint256S("0x000000000021e0dfce3f583a02bda36d90402337ee1f5f626d137b3052230ada");
        consensus.BIP65Height = 219382; // 00000000000076d8fcea02ec0963de4abfd01e771fec0863f960c2c64fe6f357
        consensus.BIP66Height = 145817; // 00000000000b1fa2dfa312863570e13fae9ca7b5566cb27e55422620b469aefa
        consensus.DIP0001Height = 259204;
        consensus.DIP0003Height = 307200;
        consensus.DIP0003EnforcementHeight = 307200;
        consensus.DIP0003EnforcementHash = uint256S("00000000000006104d2de5f4076b6be6fa2f6204186ae554c5cafac2c46f72e4");
        consensus.DIP0008Height = 329600; // 00000000000000112e41e4b3afda8b233b8cc07c532d2eac5de097b68358c43e
        consensus.BRRHeight = std::numeric_limits<int>::max(); // LKSCOIN: block reward reallocation never activates (flat 80% to masternodes)
        consensus.MinBIP9WarningHeight = 1090656; // dip8 activation height + miner confirmation window
        consensus.powLimit = uint256S("00000fffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"); // ~uint256(0) >> 20
        consensus.nPowTargetTimespan = 24 * 60 * 60; // Lks: 1 day
        consensus.nPowTargetSpacing = 2.5 * 60; // Lks: 2.5 minutes
        consensus.fPowAllowMinDifficultyBlocks = false;
        consensus.fPowNoRetargeting = false;
        consensus.nPowKGWHeight = 15200;
        consensus.nPowDGWHeight = 34140;
        consensus.nRuleChangeActivationThreshold = 1916; // 95% of 2016
        consensus.nMinerConfirmationWindow = 2016; // nPowTargetTimespan / nPowTargetSpacing
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1199145601; // January 1, 2008
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1632960000; // December 31, 2008

        // Deployment of BIP68, BIP112, and BIP113.
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = 1486252800; // Feb 5th, 2017
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = 999999999999ULL; // Feb 5th, 2018

        // Deployment of DIP0001
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].bit = 1;
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nStartTime = 1575214541; // Oct 15th, 2017
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nTimeout = 999999999999ULL; // Oct 15th, 2018
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nWindowSize = 800;
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nThresholdStart = 640; // 80% of 4032

        // Deployment of BIP147
        consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].bit = 2;
        consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].nStartTime = 1575214541; // Apr 23th, 2018
        consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].nTimeout = 999999999999ULL; // Apr 23th, 2019
        consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].nWindowSize = 800;
        consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].nThresholdStart = 640; // 80% of 4032

        // Deployment of DIP0003
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0003].bit = 3;
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0003].nStartTime = 1597924800; // Jan 1st, 2019
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0003].nTimeout = 999999999999ULL; // Jan 1st, 2020
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0003].nWindowSize = 800;
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0003].nThresholdStart = 640; // 80% of 4032

        // Deployment of DIP0008
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0008].bit = 4;
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0008].nStartTime = 1602599280; // May 15th, 2019
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0008].nTimeout = 999999999999ULL; // May 15th, 2020
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0008].nWindowSize = 800;
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0008].nThresholdStart = 80; // 80% of 4032

        // Deployment of Block Reward Reallocation
        consensus.vDeployments[Consensus::DEPLOYMENT_REALLOC].bit = 5;
        consensus.vDeployments[Consensus::DEPLOYMENT_REALLOC].nStartTime = 1614527373; // Oct 1st, 2020
        consensus.vDeployments[Consensus::DEPLOYMENT_REALLOC].nTimeout = 1646063373; // Oct 1st, 2021
        consensus.vDeployments[Consensus::DEPLOYMENT_REALLOC].nWindowSize = 800;
        consensus.vDeployments[Consensus::DEPLOYMENT_REALLOC].nThresholdStart = 640; // 80% of 4032
        consensus.vDeployments[Consensus::DEPLOYMENT_REALLOC].nThresholdMin = 480; // 60% of 4032
        consensus.vDeployments[Consensus::DEPLOYMENT_REALLOC].nFalloffCoeff = 5; // this corresponds to 10 periods

        // Deployment of DIP0020, DIP0021 and LLMQ_100_67 quorums
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0020].bit = 6;
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0020].nStartTime = 1790812800; // July 1st, 2021
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0020].nTimeout = 1822348800; // July 1st, 2022
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0020].nWindowSize = 800;
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0020].nThresholdStart = 640; // 80% of 4032
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0020].nThresholdMin = 480; // 60% of 4032
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0020].nFalloffCoeff = 5; // this corresponds to 10 periods

        // Deployment of Quorum Rotation DIP and decreased proposal fee (Values to be determined)
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0024].bit = 7;
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0024].nStartTime = 1893456000; // Jan 1st, 2030 - dormant (LKSCOIN)
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0024].nTimeout = 999999999999ULL; // dormant (LKSCOIN)
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0024].nWindowSize = 4032;
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0024].nThresholdStart = 3226; // 80% of 4032
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0024].nThresholdMin = 2420;   // 60% of 4032
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0024].nFalloffCoeff = 5;      // this corresponds to 10 periods

        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = uint256S("0x00"); // 1796500

        // By default assume that the signatures in ancestors of this block are valid.
        consensus.defaultAssumeValid = uint256S("0x000000000000003a70e6370e286bad2940869ec9fd327359c6eca25202ba89d8"); // 1796500

        /**
         * The message start string is designed to be unlikely to occur in normal data.
         * The characters are rarely used upper ASCII, not valid as UTF-8, and produce
         * a large 32-bit integer with any alignment.
         */
        pchMessageStart[0] = 0xbf;
        pchMessageStart[1] = 0x0c;
        pchMessageStart[2] = 0x6b;
        pchMessageStart[3] = 0xbd;
        nDefaultPort = 9400;
        nPruneAfterHeight = 100000;
        m_assumed_blockchain_size = 45;
        m_assumed_chain_state_size = 1;

        genesis = CreateGenesisBlock(1496594048, 786953, 0x1e0ffff0, 1, 1000 * COIN);
        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock == uint256S("0x00000c1d4fdbe9495dfbade468156a567db3e11855bcab75808f8674fa6ea060"));
        assert(genesis.hashMerkleRoot == uint256S("0x0a6b4c0d069c0eb809e0ca6f0682dcb8619b3abe64f529fa60dd446843290e24"));

        // Note that of those which support the service bits prefix, most only support a subset of
        // possible options.
        // This is fine at runtime as we'll fall back to using them as a oneshot if they don't support the
        // service bits we want, but we should get them updated to support all service bits wanted by any
        // release ASAP to avoid it where possible.
        // DNS seeds: plain A records (NO Cloudflare proxy - a proxied hostname only
        // resolves to CDN addresses and is useless as a seed) pointing at nodes
        // listening on port 9400.
        vSeeds.emplace_back("seed.lkschain.io");
        vSeeds.emplace_back("seed1.lkschain.io");
        vSeeds.emplace_back("seed2.lkschain.io");
        vSeeds.emplace_back("seed3.lkschain.io");
        vSeeds.emplace_back("seed4.lkschain.io");
        vSeeds.emplace_back("seed5.lkschain.io");
        vSeeds.emplace_back("seed6.lkschain.io");

        // Lks addresses start with 'X'
        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,76);
        // Lks script addresses start with '7'
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,16);
        // Lks private keys start with '7' or 'X'
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,204);
        // Lks BIP32 pubkeys start with 'xpub' (Bitcoin defaults)
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x88, 0xB2, 0x1E};
        // Lks BIP32 prvkeys start with 'xprv' (Bitcoin defaults)
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x88, 0xAD, 0xE4};

        // Lks BIP44 coin type is '5'
        nExtCoinType = 5;

        vFixedSeeds = std::vector<SeedSpec6>(std::begin(pnSeed6_main), std::end(pnSeed6_main));

        // long living quorum params
        AddLLMQ(Consensus::LLMQType::LLMQ_50_60);
        AddLLMQ(Consensus::LLMQType::LLMQ_LKS_10_60); // gated by nLKSSmallQuorumHeight
        AddLLMQ(Consensus::LLMQType::LLMQ_60_75);
        AddLLMQ(Consensus::LLMQType::LLMQ_400_60);
        AddLLMQ(Consensus::LLMQType::LLMQ_400_85);
        AddLLMQ(Consensus::LLMQType::LLMQ_100_67);
        consensus.llmqTypeChainLocks = Consensus::LLMQType::LLMQ_LKS_10_60;
        consensus.llmqTypeInstantSend = Consensus::LLMQType::LLMQ_50_60;
        consensus.llmqTypeDIP0024InstantSend = Consensus::LLMQType::LLMQ_60_75;
        consensus.llmqTypePlatform = Consensus::LLMQType::LLMQ_100_67;
        consensus.llmqTypeMnhf = Consensus::LLMQType::LLMQ_400_85;

        fDefaultConsistencyChecks = false;
        fRequireStandard = true;
        fRequireRoutableExternalIP = true;
        m_is_test_chain = false;
        fAllowMultipleAddressesFromGroup = false;
        fAllowMultiplePorts = false;
        nLLMQConnectionRetryTimeout = 60;
        m_is_mockable_chain = false;

        nPoolMinParticipants = 3;
        nPoolMaxParticipants = 20;
        nFulfilledRequestExpireTime = 60*60; // fulfilled requests expire in 1 hour

        vSporkAddresses = {"XdBu2FTWQRqeZCShnYtJ37fHVRaZPXLtzw"};
        nMinSporkKeys = 1;
        fBIP9CheckMasternodesUpgraded = true;

        checkpointData = {
            {
                {0, uint256S("0x00000c1d4fdbe9495dfbade468156a567db3e11855bcab75808f8674fa6ea060")},
                {100, uint256S("0x00000f4352f34460b24f0d80e2ad0911931042b2f16828b98e9dd60dcd9910fa")},
                {600, uint256S("0x00000000008fbbc9694ed089e5190782d55465d8b8ab33b83d3d4c8fab7e0160")},
                {11111, uint256S("0x000000000051bcd5ad9a2e427a21c8b5fc91a261e7bef428839e91e2bfa9e7bb")},
                {22222, uint256S("0x0000000000510d074adcb7567278756a6b868a6da45bcbad069e81198cb4cbdb")},
                {44444, uint256S("0x000000000010c4da72abea2af7b94ceeec52350ada878bdbf2de1e3a0b4dc162")},
                {88888, uint256S("0x000000000044ec96756cc52d9d6f864f2f1f72fa3e83efc55ab442ab6ce10856")},
                {111111, uint256S("0x000000000003b840a117d90721384f0b93a9cc2afeda166dd57da8b469fa0745")},
                {222222, uint256S("0x0000000000000a02963337cb7968590ac830a911295853686baac0392d93c448")},
                {330333, uint256S("0x0000000000004066f7014d7e64069d14abaf58f3a7acb36da6d7fa08c8f8138b")},
                {390800, uint256S("0x00000000000191b2fb1a03d70d3b7d67a7579439fddc319eeec273b3f8d155a8")},
            }
        };

        // getchaintxstats 17280 000000000000001d531f36005159f19351bd49ca676398a561e55dcccb84eacd
        chainTxData = ChainTxData{
            1614527373, 
        };
    }
};

/**
 * Testnet (v3)
 */
class CTestNetParams : public CChainParams {
public:
    CTestNetParams() {
        strNetworkID = CBaseChainParams::TESTNET;
        consensus.nSubsidyHalvingInterval = 210240;
        consensus.nMasternodePaymentsStartBlock = 4010; // not true, but it's ok as long as it's less then nMasternodePaymentsIncreaseBlock
        consensus.nMasternodePaymentsIncreaseBlock = 4030;
        consensus.nMasternodePaymentsIncreasePeriod = 10;
        consensus.nInstantSendConfirmationsRequired = 2;
        consensus.nInstantSendKeepLock = 6;
        consensus.nBudgetPaymentsStartBlock = 4100;
        consensus.nBudgetPaymentsCycleBlocks = 50;
        consensus.nBudgetPaymentsWindowBlocks = 10;
        consensus.nSuperblockStartBlock = 4200; // NOTE: Should satisfy nSuperblockStartBlock > nBudgetPeymentsStartBlock
        consensus.nSuperblockStartHash = uint256S("0x000000006d3dbf661c66dad0745dad2d55ac514dbb4d01f18c1cb114fe743799"); // do not check this on testnet
        consensus.nSuperblockCycle = 24; // Superblocks can be issued hourly on testnet
        consensus.nSuperblockMaturityWindow = 24; // This is equal to SB cycle on testnet
        consensus.nGovernanceMinQuorum = 1;
        consensus.nGovernanceFilterElements = 500;
        consensus.nMasternodeMinimumConfirmations = 1;
        consensus.BIP34Height = 76;
        consensus.BIP34Hash = uint256S("0x000003af47e0f59ba4995d25e8761f52c9297fc7dc8a45a9468f262be03c25cb");
        consensus.BIP65Height = 2431; // 0000039cf01242c7f921dcb4806a5994bc003b48c1973ae0c89b67809c2bb2ab
        consensus.BIP66Height = 2075; // 0000002acdd29a14583540cb72e1c5cc83783560e38fa7081495d474fe1671f7
        consensus.DIP0001Height = 4430;
        consensus.DIP0003Height = 4090;
        consensus.DIP0003EnforcementHeight = 4090;
        consensus.DIP0003EnforcementHash = uint256S("000005400cc3a7e06c4569cc3078d0c88d0e934ff15d8baa7f1af705fa0b1a50");
        consensus.DIP0008Height = 0; // 000000000e9329d964d80e7dab2e704b43b6bd2b91fea1e9315d38932e55fb55
        consensus.BRRHeight = std::numeric_limits<int>::max(); // LKSCOIN: block reward reallocation never activates (flat 80% to masternodes)
        consensus.MinBIP9WarningHeight = 80816;  // dip8 activation height + miner confirmation window
        consensus.powLimit = uint256S("00000fffff000000000000000000000000000000000000000000000000000000"); // ~uint256(0) >> 20
        consensus.nPowTargetTimespan = 24 * 60 * 60; // Lks: 1 day
        consensus.nPowTargetSpacing = 2.5 * 60; // Lks: 2.5 minutes
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting = false;
        consensus.nPowKGWHeight = 4001; // nPowKGWHeight >= nPowDGWHeight means "no KGW"
        consensus.nPowDGWHeight = 4001; // TODO: make sure to drop all spork6 related code on next testnet reset
        consensus.nRuleChangeActivationThreshold = 1512; // 75% for testchains
        consensus.nMinerConfirmationWindow = 2016; // nPowTargetTimespan / nPowTargetSpacing
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1199145601; // January 1, 2008
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1230767999; // December 31, 2008

        // Deployment of BIP68, BIP112, and BIP113.
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = 1599350400; // Dec 13th, 2018
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = 999999999999ULL;

        // Deployment of DIP0001
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].bit = 1;
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nStartTime = 1599350400; // Dec 13th, 2018
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nTimeout = 999999999999ULL;
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nWindowSize = 10;
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nThresholdStart = 5; // 50% of 100

        // Deployment of BIP147
        consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].bit = 2;
        consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].nStartTime = 1599350400; // Dec 13th, 2018
        consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].nTimeout = 999999999999ULL;
        consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].nWindowSize = 10;
        consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].nThresholdStart = 5; // 50% of 100

        // Deployment of DIP0003
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0003].bit = 3;
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0003].nStartTime = 1599350400; // Dec 13th, 2018
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0003].nTimeout = 999999999999ULL;
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0003].nWindowSize = 10;
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0003].nThresholdStart = 5; // 50% of 100

        // Deployment of DIP0008
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0008].bit = 4;
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0008].nStartTime = 1602599280; // Mar 21st, 2019
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0008].nTimeout = 999999999999ULL;
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0008].nWindowSize = 10;
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0008].nThresholdStart = 5; // 50% of 100

        // Deployment of Block Reward Reallocation
        consensus.vDeployments[Consensus::DEPLOYMENT_REALLOC].bit = 5;
        consensus.vDeployments[Consensus::DEPLOYMENT_REALLOC].nStartTime = 1598918400; // Sep 1st, 2020
        consensus.vDeployments[Consensus::DEPLOYMENT_REALLOC].nTimeout = 1630454400;
        consensus.vDeployments[Consensus::DEPLOYMENT_REALLOC].nWindowSize = 10;
        consensus.vDeployments[Consensus::DEPLOYMENT_REALLOC].nThresholdStart = 5; // 80% of 100
        consensus.vDeployments[Consensus::DEPLOYMENT_REALLOC].nThresholdMin = 4; // 60% of 100
        consensus.vDeployments[Consensus::DEPLOYMENT_REALLOC].nFalloffCoeff = 2; // this corresponds to 10 periods

        // Deployment of DIP0020, DIP0021 and LLMQ_100_67 quorums
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0020].bit = 6;
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0020].nStartTime = 1893456000; // December 1st, 2020
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0020].nTimeout = 999999999999ULL; // December 1st, 2021
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0020].nWindowSize = 100;
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0020].nThresholdStart = 80; // 80% of 100
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0020].nThresholdMin = 60; // 60% of 100
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0020].nFalloffCoeff = 5; // this corresponds to 10 periods

        // Deployment of Quorum Rotation DIP and decreased proposal fee
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0024].bit = 7;
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0024].nStartTime = 1893456000; // Jan 1st, 2030 - dormant (LKSCOIN)
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0024].nTimeout = 999999999999ULL; // dormant (LKSCOIN)
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0024].nWindowSize = 100;
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0024].nThresholdStart = 80; // 80% of 100
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0024].nThresholdMin = 60;   // 60% of 100
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0024].nFalloffCoeff = 5;      // this corresponds to 10 periods

        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = uint256S("0x00"); // 808000

        // By default assume that the signatures in ancestors of this block are valid.
        consensus.defaultAssumeValid = uint256S("0x000000074decb2119c44a9e65d005834fc69ffd31f8ab34316361875a4b25020"); // 808000

        pchMessageStart[0] = 0xce;
        pchMessageStart[1] = 0xe2;
        pchMessageStart[2] = 0xca;
        pchMessageStart[3] = 0xff;
        nDefaultPort = 19400;
        nPruneAfterHeight = 1000;
        m_assumed_blockchain_size = 4;
        m_assumed_chain_state_size = 1;

        genesis = CreateGenesisBlock(1496594049L, 2770643L, 0x1e0ffff0, 1, 1000 * COIN);
        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock == uint256S("0x0000068afbbc76761a1b3b5d8cccc42907da1cca679628393bd35082d4a15e48"));
        assert(genesis.hashMerkleRoot == uint256S("0x0a6b4c0d069c0eb809e0ca6f0682dcb8619b3abe64f529fa60dd446843290e24"));

        vFixedSeeds.clear();
        // LKSCOIN testnet is not currently operating: no fixed seeds.
        vFixedSeeds.clear();

        vSeeds.clear();
        // nodes with support for servicebits filtering should be at the top
        vSeeds.emplace_back("fork.lkschain.io"); // LKSCOIN testnet (not currently operating)

        // Testnet Lks addresses start with 'y'
        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,140);
        // Testnet Lks script addresses start with '8' or '9'
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,19);
        // Testnet private keys start with '9' or 'c' (Bitcoin defaults)
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
        // Testnet Lks BIP32 pubkeys start with 'tpub' (Bitcoin defaults)
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x35, 0x87, 0xCF};
        // Testnet Lks BIP32 prvkeys start with 'tprv' (Bitcoin defaults)
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x35, 0x83, 0x94};

        // Testnet Lks BIP44 coin type is '1' (All coin's testnet default)
        nExtCoinType = 1;

        // long living quorum params
        AddLLMQ(Consensus::LLMQType::LLMQ_50_60);
        AddLLMQ(Consensus::LLMQType::LLMQ_LKS_10_60); // gated by nLKSSmallQuorumHeight
        AddLLMQ(Consensus::LLMQType::LLMQ_60_75);
        AddLLMQ(Consensus::LLMQType::LLMQ_400_60);
        AddLLMQ(Consensus::LLMQType::LLMQ_400_85);
        AddLLMQ(Consensus::LLMQType::LLMQ_100_67);
        AddLLMQ(Consensus::LLMQType::LLMQ_25_67);
        consensus.llmqTypeChainLocks = Consensus::LLMQType::LLMQ_50_60;
        consensus.llmqTypeInstantSend = Consensus::LLMQType::LLMQ_50_60;
        consensus.llmqTypeDIP0024InstantSend = Consensus::LLMQType::LLMQ_60_75;
        consensus.llmqTypePlatform = Consensus::LLMQType::LLMQ_25_67;
        consensus.llmqTypeMnhf = Consensus::LLMQType::LLMQ_50_60;

        fDefaultConsistencyChecks = false;
        fRequireStandard = false;
        fRequireRoutableExternalIP = true;
        m_is_test_chain = true;
        fAllowMultipleAddressesFromGroup = false;
        fAllowMultiplePorts = true;
        nLLMQConnectionRetryTimeout = 60;
        m_is_mockable_chain = false;

        nPoolMinParticipants = 2;
        nPoolMaxParticipants = 20;
        nFulfilledRequestExpireTime = 5*60; // fulfilled requests expire in 5 minutes

        vSporkAddresses = {"yVKKPX2kDWnMMNDap7tMKxKT8wwBsRcmSK"};
        nMinSporkKeys = 1;
        fBIP9CheckMasternodesUpgraded = true;

        checkpointData = {
            {   
                {0, uint256S("0x0000068afbbc76761a1b3b5d8cccc42907da1cca679628393bd35082d4a15e48")},
                {4200, uint256S("0x000000006d3dbf661c66dad0745dad2d55ac514dbb4d01f18c1cb114fe743799")},
                {4055, uint256S("0x000000003b94f7004a28994d1b040fa77add3889089ab5b0b03bb6e1bc21b942")},
            }
        };

        // getchaintxstats 17280 00000104cb60a2b5e00a8a4259582756e5bf0dca201c0993c63f0e54971ea91a
        chainTxData = ChainTxData{
            
        };
    }
};

/**
 * Devnet
 */
class CDevNetParams : public CChainParams {
public:
    explicit CDevNetParams(const ArgsManager& args) {
        strNetworkID = CBaseChainParams::DEVNET;
        consensus.nSubsidyHalvingInterval = 210240;
        consensus.nMasternodePaymentsStartBlock = 4010; // not true, but it's ok as long as it's less then nMasternodePaymentsIncreaseBlock
        consensus.nMasternodePaymentsIncreaseBlock = 4030;
        consensus.nMasternodePaymentsIncreasePeriod = 10;
        consensus.nInstantSendConfirmationsRequired = 2;
        consensus.nInstantSendKeepLock = 6;
        consensus.nBudgetPaymentsStartBlock = 4100;
        consensus.nBudgetPaymentsCycleBlocks = 50;
        consensus.nBudgetPaymentsWindowBlocks = 10;
        consensus.nSuperblockStartBlock = 4200; // NOTE: Should satisfy nSuperblockStartBlock > nBudgetPeymentsStartBlock
        consensus.nSuperblockStartHash = uint256(); // do not check this on devnet
        consensus.nSuperblockCycle = 24; // Superblocks can be issued hourly on devnet
        consensus.nSuperblockMaturityWindow = 24; // This is equal to SB cycle on devnet
        consensus.nGovernanceMinQuorum = 1;
        consensus.nGovernanceFilterElements = 500;
        consensus.nMasternodeMinimumConfirmations = 1;
        consensus.BIP34Height = 1; // BIP34 activated immediately on devnet
        consensus.BIP65Height = 1; // BIP65 activated immediately on devnet
        consensus.BIP66Height = 1; // BIP66 activated immediately on devnet
        consensus.DIP0001Height = 2; // DIP0001 activated immediately on devnet
        consensus.DIP0003Height = 2; // DIP0003 activated immediately on devnet
        consensus.DIP0003EnforcementHeight = 2; // DIP0003 activated immediately on devnet
        consensus.DIP0003EnforcementHash = uint256();
        consensus.DIP0008Height = 2; // DIP0008 activated immediately on devnet
        consensus.BRRHeight = std::numeric_limits<int>::max(); // LKSCOIN: block reward reallocation never activates (flat 80% to masternodes)
        consensus.MinBIP9WarningHeight = 2018; // dip8 activation height + miner confirmation window
        consensus.powLimit = uint256S("7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"); // ~uint256(0) >> 1
        consensus.nPowTargetTimespan = 24 * 60 * 60; // Lks: 1 day
        consensus.nPowTargetSpacing = 2.5 * 60; // Lks: 2.5 minutes
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting = false;
        consensus.nPowKGWHeight = 4001; // nPowKGWHeight >= nPowDGWHeight means "no KGW"
        consensus.nPowDGWHeight = 4001;
        consensus.nRuleChangeActivationThreshold = 1512; // 75% for testchains
        consensus.nMinerConfirmationWindow = 2016; // nPowTargetTimespan / nPowTargetSpacing
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1199145601; // January 1, 2008
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1230767999; // December 31, 2008

        // Deployment of BIP68, BIP112, and BIP113.
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = 1575214541; // Dic 1th, 2019 
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = 999999999999ULL;

        // Deployment of DIP0001
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].bit = 1;
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nStartTime = 1599350400; // Sept 6th, 2020 
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nTimeout = 999999999999ULL;
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nWindowSize = 10;
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nThresholdStart = 5; // 50% of 10

        // Deployment of BIP147
        consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].bit = 2;
        consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].nStartTime = 1599350400; // Sept 6th, 2020 
        consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].nTimeout = 999999999999ULL;
        consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].nWindowSize = 10;
        consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].nThresholdStart = 5; // 50% of 10

        // Deployment of DIP0003
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0003].bit = 3;
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0003].nStartTime = 1535752800; // Sep 1st, 2018
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0003].nTimeout = 999999999999ULL;
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0003].nWindowSize = 100;
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0003].nThresholdStart = 50; // 50% of 100

        // Deployment of DIP0008
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0008].bit = 4;
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0008].nStartTime = 1602599280; // Mar 21st, 2019
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0008].nTimeout = 999999999999ULL;
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0008].nWindowSize = 10;
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0008].nThresholdStart = 5; // 50% of 100

        // Deployment of Block Reward Reallocation
        consensus.vDeployments[Consensus::DEPLOYMENT_REALLOC].bit = 5;
        consensus.vDeployments[Consensus::DEPLOYMENT_REALLOC].nStartTime = 1598918400; // Sep 1st, 2020
        consensus.vDeployments[Consensus::DEPLOYMENT_REALLOC].nTimeout = 1900281600;
        consensus.vDeployments[Consensus::DEPLOYMENT_REALLOC].nWindowSize = 10;
        consensus.vDeployments[Consensus::DEPLOYMENT_REALLOC].nThresholdStart = 5; // 80% of 100
        consensus.vDeployments[Consensus::DEPLOYMENT_REALLOC].nThresholdMin = 4; // 60% of 100
        consensus.vDeployments[Consensus::DEPLOYMENT_REALLOC].nFalloffCoeff = 2; // this corresponds to 10 periods

        // Deployment of DIP0020, DIP0021 and LLMQ_100_67 quorums
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0020].bit = 6;
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0020].nStartTime = 0; // November 1st, 2020
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0020].nTimeout = 999999999999ULL;
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0020].nWindowSize = 100;
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0020].nThresholdStart = 80; // 80% of 100
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0020].nThresholdMin = 60; // 60% of 100
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0020].nFalloffCoeff = 5; // this corresponds to 10 periods

        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0024].bit = 7;
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0024].nStartTime = 1893456000; // Jan 1st, 2030 - dormant (LKSCOIN)
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0024].nTimeout = 999999999999ULL; // dormant (LKSCOIN)
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0024].nWindowSize = 100;
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0024].nThresholdStart = 80; // 80% of 100
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0024].nThresholdMin = 60;   // 60% of 100
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0024].nFalloffCoeff = 5;    // this corresponds to 10 periods

        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = uint256S("0x00");

        // By default assume that the signatures in ancestors of this block are valid.
        consensus.defaultAssumeValid = uint256S("0x00");

        pchMessageStart[0] = 0xe2;
        pchMessageStart[1] = 0xca;
        pchMessageStart[2] = 0xff;
        pchMessageStart[3] = 0xce;
        nDefaultPort = 19799;
        nPruneAfterHeight = 1000;
        m_assumed_blockchain_size = 0;
        m_assumed_chain_state_size = 0;

        UpdateDevnetSubsidyAndDiffParametersFromArgs(args);
        // LKSCOIN: regtest and devnet keep upstream's genesis, whose nBits is the
        // trivial 0x207fffff. LKSCOIN 3.x had copied the mainnet difficulty
        // (0x1e0ffff0) into these local chains; with fPowNoRetargeting every block
        // then costs ~1M hashes (~20s each), which makes the unit tests and any
        // devnet rehearsal impractically slow. These chains are throwaway, so
        // there is no compatibility reason to keep the LKSCOIN values here.
        genesis = CreateGenesisBlock(1496594050, 530659, 0x207fffff, 1, 1000 * COIN);
        consensus.hashGenesisBlock = genesis.GetHash();
        // TODO(genesis): re-enable with the value printed by `lks-cli -regtest getblockhash 0`
        // assert(consensus.hashGenesisBlock == uint256S("..."));
        // assert(genesis.hashMerkleRoot == uint256S("..."));
        devnetGenesis = FindDevNetGenesisBlock(genesis, 50 * COIN);
        consensus.hashDevnetGenesisBlock = devnetGenesis.GetHash();

        vFixedSeeds.clear();
        vSeeds.clear();
        //vSeeds.push_back(CDNSSeedData("lksevo.org",  "devnet-seed.lksevo.org"));

        // Testnet Lks addresses start with 'y'
        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,140);
        // Testnet Lks script addresses start with '8' or '9'
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,19);
        // Testnet private keys start with '9' or 'c' (Bitcoin defaults)
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
        // Testnet Lks BIP32 pubkeys start with 'tpub' (Bitcoin defaults)
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x35, 0x87, 0xCF};
        // Testnet Lks BIP32 prvkeys start with 'tprv' (Bitcoin defaults)
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x35, 0x83, 0x94};

        // Testnet Lks BIP44 coin type is '1' (All coin's testnet default)
        nExtCoinType = 1;

        // long living quorum params
        AddLLMQ(Consensus::LLMQType::LLMQ_50_60);
        AddLLMQ(Consensus::LLMQType::LLMQ_LKS_10_60); // gated by nLKSSmallQuorumHeight
        AddLLMQ(Consensus::LLMQType::LLMQ_60_75);
        AddLLMQ(Consensus::LLMQType::LLMQ_400_60);
        AddLLMQ(Consensus::LLMQType::LLMQ_400_85);
        AddLLMQ(Consensus::LLMQType::LLMQ_100_67);
        AddLLMQ(Consensus::LLMQType::LLMQ_DEVNET);
        AddLLMQ(Consensus::LLMQType::LLMQ_DEVNET_DIP0024);
        consensus.llmqTypeChainLocks = Consensus::LLMQType::LLMQ_50_60;
        consensus.llmqTypeInstantSend = Consensus::LLMQType::LLMQ_50_60;
        consensus.llmqTypeDIP0024InstantSend = Consensus::LLMQType::LLMQ_60_75;
        consensus.llmqTypePlatform = Consensus::LLMQType::LLMQ_100_67;
        consensus.llmqTypeMnhf = Consensus::LLMQType::LLMQ_50_60;

        UpdateDevnetLLMQChainLocksFromArgs(args);
        UpdateDevnetLLMQInstantSendFromArgs(args);
        UpdateDevnetLLMQInstantSendDIP0024FromArgs(args);
        UpdateLLMQDevnetParametersFromArgs(args);
        UpdateDevnetPowTargetSpacingFromArgs(args);

        fDefaultConsistencyChecks = false;
        fRequireStandard = false;
        fRequireRoutableExternalIP = true;
        m_is_test_chain = true;
        fAllowMultipleAddressesFromGroup = true;
        fAllowMultiplePorts = true;
        nLLMQConnectionRetryTimeout = 60;
        m_is_mockable_chain = false;

        nPoolMinParticipants = 2;
        nPoolMaxParticipants = 20;
        nFulfilledRequestExpireTime = 5*60; // fulfilled requests expire in 5 minutes

        vSporkAddresses = {"yXweRfwWMV9Vciv6aD6agURwg5u8tEy7qQ"};
        nMinSporkKeys = 1;
        // devnets are started with no blocks and no MN, so we can't check for upgraded MN (as there are none)
        fBIP9CheckMasternodesUpgraded = false;

        checkpointData = (CCheckpointData) {
            {
                { 0, uint256S("0x000008ca1832a4baf228eb1553c03d3a2c8e02399550dd6ea8d65cec3ef23d2e")},
                { 1, devnetGenesis.GetHash() },
            }
        };

        chainTxData = ChainTxData{
            devnetGenesis.GetBlockTime(), // * UNIX timestamp of devnet genesis block
            2,                            // * we only have 2 coinbase transactions when a devnet is started up
            0.01                          // * estimated number of transactions per second
        };
    }

    /**
     * Allows modifying the subsidy and difficulty devnet parameters.
     */
    void UpdateDevnetSubsidyAndDiffParameters(int nMinimumDifficultyBlocks, int nHighSubsidyBlocks, int nHighSubsidyFactor)
    {
        consensus.nMinimumDifficultyBlocks = nMinimumDifficultyBlocks;
        consensus.nHighSubsidyBlocks = nHighSubsidyBlocks;
        consensus.nHighSubsidyFactor = nHighSubsidyFactor;
    }
    void UpdateDevnetSubsidyAndDiffParametersFromArgs(const ArgsManager& args);

    /**
     * Allows modifying the LLMQ type for ChainLocks.
     */
    void UpdateDevnetLLMQChainLocks(Consensus::LLMQType llmqType)
    {
        consensus.llmqTypeChainLocks = llmqType;
    }
    void UpdateDevnetLLMQChainLocksFromArgs(const ArgsManager& args);

    /**
     * Allows modifying the LLMQ type for InstantSend.
     */
    void UpdateDevnetLLMQInstantSend(Consensus::LLMQType llmqType)
    {
        consensus.llmqTypeInstantSend = llmqType;
    }

    /**
     * Allows modifying the LLMQ type for InstantSend (DIP0024).
     */
    void UpdateDevnetLLMQDIP0024InstantSend(Consensus::LLMQType llmqType)
    {
        consensus.llmqTypeDIP0024InstantSend = llmqType;
    }

    /**
     * Allows modifying PowTargetSpacing
     */
    void UpdateDevnetPowTargetSpacing(int64_t nPowTargetSpacing)
    {
        consensus.nPowTargetSpacing = nPowTargetSpacing;
    }

    /**
     * Allows modifying parameters of the devnet LLMQ
     */
    void UpdateLLMQDevnetParameters(int size, int threshold)
    {
        auto params = ranges::find_if(consensus.llmqs, [](const auto& llmq){ return llmq.type == Consensus::LLMQType::LLMQ_DEVNET;});
        assert(params != consensus.llmqs.end());
        params->size = size;
        params->minSize = threshold;
        params->threshold = threshold;
        params->dkgBadVotesThreshold = threshold;
    }
    void UpdateLLMQDevnetParametersFromArgs(const ArgsManager& args);
    void UpdateDevnetLLMQInstantSendFromArgs(const ArgsManager& args);
    void UpdateDevnetLLMQInstantSendDIP0024FromArgs(const ArgsManager& args);
    void UpdateDevnetPowTargetSpacingFromArgs(const ArgsManager& args);
};

/**
 * Regression test
 */
class CRegTestParams : public CChainParams {
public:
    explicit CRegTestParams(const ArgsManager& args) {
        strNetworkID =  CBaseChainParams::REGTEST;
        consensus.nSubsidyHalvingInterval = 150;
        consensus.nMasternodePaymentsStartBlock = 240;
        consensus.nMasternodePaymentsIncreaseBlock = 350;
        consensus.nMasternodePaymentsIncreasePeriod = 10;
        consensus.nInstantSendConfirmationsRequired = 2;
        consensus.nInstantSendKeepLock = 6;
        consensus.nBudgetPaymentsStartBlock = 1000;
        consensus.nBudgetPaymentsCycleBlocks = 50;
        consensus.nBudgetPaymentsWindowBlocks = 10;
        consensus.nSuperblockStartBlock = 1500;
        consensus.nSuperblockStartHash = uint256(); // do not check this on regtest
        consensus.nSuperblockCycle = 10;
        consensus.nSuperblockMaturityWindow = 10; // This is equal to SB cycle on regtest
        consensus.nGovernanceMinQuorum = 1;
        consensus.nGovernanceFilterElements = 100;
        consensus.nMasternodeMinimumConfirmations = 1;
        consensus.BIP34Height = 100000000; // BIP34 activated on regtest (Used in functional tests)
        consensus.BIP34Hash = uint256();
        consensus.BIP65Height = 1351; // BIP65 activated on regtest (Used in functional tests)
        consensus.BIP66Height = 1251; // BIP66 activated on regtest (Used in functional tests)
        consensus.DIP0001Height = 2000;
        consensus.DIP0003Height = 432;
        consensus.DIP0003EnforcementHeight = 500;
        consensus.DIP0003EnforcementHash = uint256();
        consensus.DIP0008Height = 432;
        consensus.BRRHeight = std::numeric_limits<int>::max(); // LKSCOIN: block reward reallocation never activates (flat 80% to masternodes)
        consensus.MinBIP9WarningHeight = 0;
        consensus.powLimit = uint256S("7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"); // ~uint256(0) >> 1
        consensus.nPowTargetTimespan = 24 * 60 * 60; // Lks: 1 day
        consensus.nPowTargetSpacing = 2.5 * 60; // Lks: 2.5 minutes
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting = true;
        consensus.nPowKGWHeight = 15200; // same as mainnet
        consensus.nPowDGWHeight = 34140; // same as mainnet
        consensus.nRuleChangeActivationThreshold = 108; // 75% for testchains
        consensus.nMinerConfirmationWindow = 144; // Faster than normal for regtest (144 instead of 2016)
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 999999999999ULL;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = 999999999999ULL;
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].bit = 1;
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nTimeout = 999999999999ULL;
        consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].bit = 2;
        consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].nTimeout = 999999999999ULL;
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0003].bit = 3;
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0003].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0003].nTimeout = 999999999999ULL;
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0008].bit = 4;
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0008].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0008].nTimeout = 999999999999ULL;
        consensus.vDeployments[Consensus::DEPLOYMENT_REALLOC].bit = 5;
        consensus.vDeployments[Consensus::DEPLOYMENT_REALLOC].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_REALLOC].nTimeout = 999999999999ULL;
        consensus.vDeployments[Consensus::DEPLOYMENT_REALLOC].nWindowSize = 500;
        consensus.vDeployments[Consensus::DEPLOYMENT_REALLOC].nThresholdStart = 400; // 80%
        consensus.vDeployments[Consensus::DEPLOYMENT_REALLOC].nThresholdMin = 300; // 60%
        consensus.vDeployments[Consensus::DEPLOYMENT_REALLOC].nFalloffCoeff = 5;
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0020].bit = 6;
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0020].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0020].nTimeout = 999999999999ULL;
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0020].nWindowSize = 100;
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0020].nThresholdStart = 80;
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0020].nThresholdMin = 60;
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0020].nFalloffCoeff = 5;

        // Deployment of Quorum Rotation DIP and decreased proposal fee
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0024].bit = 7;
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0024].nStartTime = 1893456000; // Jan 1st, 2030 - dormant (LKSCOIN)
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0024].nTimeout = 999999999999ULL; // dormant (LKSCOIN)
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0024].nWindowSize = 300;
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0024].nThresholdStart = 240; // 80% of 300
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0024].nThresholdMin = 180;   // 60% of 300
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0024].nFalloffCoeff = 5;     // this corresponds to 10 periods

        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = uint256S("0x00");

        // By default assume that the signatures in ancestors of this block are valid.
        consensus.defaultAssumeValid = uint256S("0x00");

        pchMessageStart[0] = 0xfc;
        pchMessageStart[1] = 0xc1;
        pchMessageStart[2] = 0xb7;
        pchMessageStart[3] = 0xdc;
        nDefaultPort = 19899;
        nPruneAfterHeight = 1000;
        m_assumed_blockchain_size = 0;
        m_assumed_chain_state_size = 0;

        UpdateVersionBitsParametersFromArgs(args);
        UpdateDIP3ParametersFromArgs(args);
        UpdateDIP8ParametersFromArgs(args);
        UpdateMNPurgeParametersFromArgs(args);
        UpdateLKSSmallQuorumHeightFromArgs(args);
        UpdateBudgetParametersFromArgs(args);

        // LKSCOIN: regtest and devnet keep upstream's genesis, whose nBits is the
        // trivial 0x207fffff. LKSCOIN 3.x had copied the mainnet difficulty
        // (0x1e0ffff0) into these local chains; with fPowNoRetargeting every block
        // then costs ~1M hashes (~20s each), which makes the unit tests and any
        // devnet rehearsal impractically slow. These chains are throwaway, so
        // there is no compatibility reason to keep the LKSCOIN values here.
        genesis = CreateGenesisBlock(1496594050, 530659, 0x207fffff, 1, 1000 * COIN); 
        consensus.hashGenesisBlock = genesis.GetHash();
        // TODO(genesis): re-enable with the value printed by `lks-cli -regtest getblockhash 0`
        // assert(consensus.hashGenesisBlock == uint256S("..."));
        // assert(genesis.hashMerkleRoot == uint256S("..."));
        vFixedSeeds.clear(); //!< Regtest mode doesn't have any fixed seeds.
        vSeeds.clear();      //!< Regtest mode doesn't have any DNS seeds.

        fDefaultConsistencyChecks = true;
        fRequireStandard = true;
        fRequireRoutableExternalIP = false;
        m_is_test_chain = true;
        fAllowMultipleAddressesFromGroup = true;
        fAllowMultiplePorts = true;
        nLLMQConnectionRetryTimeout = 1; // must be lower then the LLMQ signing session timeout so that tests have control over failing behavior
        m_is_mockable_chain = true;

        nFulfilledRequestExpireTime = 5*60; // fulfilled requests expire in 5 minutes
        nPoolMinParticipants = 2;
        nPoolMaxParticipants = 20;

        
        vSporkAddresses = {"yjM9Vz8RRzZHbUdCRUVT6sehdRhgbjhNfJ"}; 
        nMinSporkKeys = 1;
        // regtest usually has no masternodes in most tests, so don't check for upgraged MNs
        fBIP9CheckMasternodesUpgraded = false;

        checkpointData = {
            {
                {0, uint256S("0x000002aaadf6458647d201d3d84469a8f2cfa9dc5a6b7025b5ad306a473fc681")}, 
            }
        };

        chainTxData = ChainTxData{
            0,
            0,
            0
        };

        // Regtest Lks addresses start with 'y'
        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,140);
        // Regtest Lks script addresses start with '8' or '9'
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,19);
        // Regtest private keys start with '9' or 'c' (Bitcoin defaults)
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
        // Regtest Lks BIP32 pubkeys start with 'tpub' (Bitcoin defaults)
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x35, 0x87, 0xCF};
        // Regtest Lks BIP32 prvkeys start with 'tprv' (Bitcoin defaults)
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x35, 0x83, 0x94};

        // Regtest Lks BIP44 coin type is '1' (All coin's testnet default)
        nExtCoinType = 1;

        // long living quorum params
        AddLLMQ(Consensus::LLMQType::LLMQ_TEST);
        AddLLMQ(Consensus::LLMQType::LLMQ_TEST_INSTANTSEND);
        AddLLMQ(Consensus::LLMQType::LLMQ_TEST_V17);
        AddLLMQ(Consensus::LLMQType::LLMQ_TEST_DIP0024);
        consensus.llmqTypeChainLocks = Consensus::LLMQType::LLMQ_TEST;
        consensus.llmqTypeInstantSend = Consensus::LLMQType::LLMQ_TEST;
        consensus.llmqTypeDIP0024InstantSend = Consensus::LLMQType::LLMQ_TEST_DIP0024;
        consensus.llmqTypePlatform = Consensus::LLMQType::LLMQ_TEST;
        consensus.llmqTypeMnhf = Consensus::LLMQType::LLMQ_TEST;

        UpdateLLMQTestParametersFromArgs(args, Consensus::LLMQType::LLMQ_TEST);
        UpdateLLMQTestParametersFromArgs(args, Consensus::LLMQType::LLMQ_TEST_INSTANTSEND);
    }

    /**
     * Allows modifying the Version Bits regtest parameters.
     */
    void UpdateVersionBitsParameters(Consensus::DeploymentPos d, int64_t nStartTime, int64_t nTimeout, int64_t nWindowSize, int64_t nThresholdStart, int64_t nThresholdMin, int64_t nFalloffCoeff)
    {
        consensus.vDeployments[d].nStartTime = nStartTime;
        consensus.vDeployments[d].nTimeout = nTimeout;
        if (nWindowSize != -1) {
            consensus.vDeployments[d].nWindowSize = nWindowSize;
        }
        if (nThresholdStart != -1) {
            consensus.vDeployments[d].nThresholdStart = nThresholdStart;
        }
        if (nThresholdMin != -1) {
            consensus.vDeployments[d].nThresholdMin = nThresholdMin;
        }
        if (nFalloffCoeff != -1) {
            consensus.vDeployments[d].nFalloffCoeff = nFalloffCoeff;
        }
    }
    void UpdateVersionBitsParametersFromArgs(const ArgsManager& args);

    /**
     * Allows modifying the DIP3 activation and enforcement height
     */
    void UpdateDIP3Parameters(int nActivationHeight, int nEnforcementHeight)
    {
        consensus.DIP0003Height = nActivationHeight;
        consensus.DIP0003EnforcementHeight = nEnforcementHeight;
    }
    void UpdateDIP3ParametersFromArgs(const ArgsManager& args);

    /**
     * Allows modifying the DIP8 activation height
     */
    void UpdateDIP8Parameters(int nActivationHeight)
    {
        consensus.DIP0008Height = nActivationHeight;
    }

    /**
     * LKSCOIN network revival parameters. Regtest only: lets the masternode
     * purge and the small quorum type be exercised without recompiling.
     */
    void UpdateMNPurgeParameters(int nStartHeight, int nPurgeHeight)
    {
        consensus.nMNPurgeStartHeight = nStartHeight;
        consensus.nMNPurgeHeight = nPurgeHeight;
    }
    void UpdateMNPurgeParametersFromArgs(const ArgsManager& args);

    void UpdateLKSSmallQuorumHeight(int nActivationHeight)
    {
        consensus.nLKSSmallQuorumHeight = nActivationHeight;
    }
    void UpdateLKSSmallQuorumHeightFromArgs(const ArgsManager& args);
    void UpdateDIP8ParametersFromArgs(const ArgsManager& args);

    /**
     * Allows modifying the budget regtest parameters.
     */
    void UpdateBudgetParameters(int nMasternodePaymentsStartBlock, int nBudgetPaymentsStartBlock, int nSuperblockStartBlock)
    {
        consensus.nMasternodePaymentsStartBlock = nMasternodePaymentsStartBlock;
        consensus.nBudgetPaymentsStartBlock = nBudgetPaymentsStartBlock;
        consensus.nSuperblockStartBlock = nSuperblockStartBlock;
    }
    void UpdateBudgetParametersFromArgs(const ArgsManager& args);

    /**
     * Allows modifying parameters of the test LLMQ
     */
    void UpdateLLMQTestParameters(int size, int threshold, const Consensus::LLMQType llmqType)
    {
        auto params = ranges::find_if(consensus.llmqs, [llmqType](const auto& llmq){ return llmq.type == llmqType;});
        assert(params != consensus.llmqs.end());
        params->size = size;
        params->minSize = threshold;
        params->threshold = threshold;
        params->dkgBadVotesThreshold = threshold;
    }
    void UpdateLLMQTestParametersFromArgs(const ArgsManager& args, const Consensus::LLMQType llmqType);
};

void CRegTestParams::UpdateVersionBitsParametersFromArgs(const ArgsManager& args)
{
    if (!args.IsArgSet("-vbparams")) return;

    for (const std::string& strDeployment : args.GetArgs("-vbparams")) {
        std::vector<std::string> vDeploymentParams;
        boost::split(vDeploymentParams, strDeployment, boost::is_any_of(":"));
        if (vDeploymentParams.size() != 3 && vDeploymentParams.size() != 5 && vDeploymentParams.size() != 7) {
            throw std::runtime_error("Version bits parameters malformed, expecting "
                    "<deployment>:<start>:<end> or "
                    "<deployment>:<start>:<end>:<window>:<threshold> or "
                    "<deployment>:<start>:<end>:<window>:<thresholdstart>:<thresholdmin>:<falloffcoeff>");
        }
        int64_t nStartTime, nTimeout, nWindowSize = -1, nThresholdStart = -1, nThresholdMin = -1, nFalloffCoeff = -1;
        if (!ParseInt64(vDeploymentParams[1], &nStartTime)) {
            throw std::runtime_error(strprintf("Invalid nStartTime (%s)", vDeploymentParams[1]));
        }
        if (!ParseInt64(vDeploymentParams[2], &nTimeout)) {
            throw std::runtime_error(strprintf("Invalid nTimeout (%s)", vDeploymentParams[2]));
        }
        if (vDeploymentParams.size() >= 5) {
            if (!ParseInt64(vDeploymentParams[3], &nWindowSize)) {
                throw std::runtime_error(strprintf("Invalid nWindowSize (%s)", vDeploymentParams[3]));
            }
            if (!ParseInt64(vDeploymentParams[4], &nThresholdStart)) {
                throw std::runtime_error(strprintf("Invalid nThresholdStart (%s)", vDeploymentParams[4]));
            }
        }
        if (vDeploymentParams.size() == 7) {
            if (!ParseInt64(vDeploymentParams[5], &nThresholdMin)) {
                throw std::runtime_error(strprintf("Invalid nThresholdMin (%s)", vDeploymentParams[5]));
            }
            if (!ParseInt64(vDeploymentParams[6], &nFalloffCoeff)) {
                throw std::runtime_error(strprintf("Invalid nFalloffCoeff (%s)", vDeploymentParams[6]));
            }
        }
        bool found = false;
        for (int j=0; j < (int)Consensus::MAX_VERSION_BITS_DEPLOYMENTS; ++j) {
            if (vDeploymentParams[0] == VersionBitsDeploymentInfo[j].name) {
                UpdateVersionBitsParameters(Consensus::DeploymentPos(j), nStartTime, nTimeout, nWindowSize, nThresholdStart, nThresholdMin, nFalloffCoeff);
                found = true;
                LogPrintf("Setting version bits activation parameters for %s to start=%ld, timeout=%ld, window=%ld, thresholdstart=%ld, thresholdmin=%ld, falloffcoeff=%ld\n",
                          vDeploymentParams[0], nStartTime, nTimeout, nWindowSize, nThresholdStart, nThresholdMin, nFalloffCoeff);
                break;
            }
        }
        if (!found) {
            throw std::runtime_error(strprintf("Invalid deployment (%s)", vDeploymentParams[0]));
        }
    }
}

void CRegTestParams::UpdateDIP3ParametersFromArgs(const ArgsManager& args)
{
    if (!args.IsArgSet("-dip3params")) return;

    std::string strParams = args.GetArg("-dip3params", "");
    std::vector<std::string> vParams;
    boost::split(vParams, strParams, boost::is_any_of(":"));
    if (vParams.size() != 2) {
        throw std::runtime_error("DIP3 parameters malformed, expecting <activation>:<enforcement>");
    }
    int nDIP3ActivationHeight, nDIP3EnforcementHeight;
    if (!ParseInt32(vParams[0], &nDIP3ActivationHeight)) {
        throw std::runtime_error(strprintf("Invalid activation height (%s)", vParams[0]));
    }
    if (!ParseInt32(vParams[1], &nDIP3EnforcementHeight)) {
        throw std::runtime_error(strprintf("Invalid enforcement height (%s)", vParams[1]));
    }
    LogPrintf("Setting DIP3 parameters to activation=%ld, enforcement=%ld\n", nDIP3ActivationHeight, nDIP3EnforcementHeight);
    UpdateDIP3Parameters(nDIP3ActivationHeight, nDIP3EnforcementHeight);
}

void CRegTestParams::UpdateDIP8ParametersFromArgs(const ArgsManager& args)
{
    if (!args.IsArgSet("-dip8params")) return;

    std::string strParams = args.GetArg("-dip8params", "");
    std::vector<std::string> vParams;
    boost::split(vParams, strParams, boost::is_any_of(":"));
    if (vParams.size() != 1) {
        throw std::runtime_error("DIP8 parameters malformed, expecting <activation>");
    }
    int nDIP8ActivationHeight;
    if (!ParseInt32(vParams[0], &nDIP8ActivationHeight)) {
        throw std::runtime_error(strprintf("Invalid activation height (%s)", vParams[0]));
    }
    LogPrintf("Setting DIP8 parameters to activation=%ld\n", nDIP8ActivationHeight);
    UpdateDIP8Parameters(nDIP8ActivationHeight);
}

void CRegTestParams::UpdateMNPurgeParametersFromArgs(const ArgsManager& args)
{
    if (!args.IsArgSet("-mnpurgeparams")) return;

    std::string strParams = args.GetArg("-mnpurgeparams", "");
    std::vector<std::string> vParams;
    boost::split(vParams, strParams, boost::is_any_of(":"));
    if (vParams.size() != 2) {
        throw std::runtime_error("Masternode purge parameters malformed, expecting <windowStart>:<purgeHeight>");
    }
    int nStartHeight, nPurgeHeight;
    if (!ParseInt32(vParams[0], &nStartHeight)) {
        throw std::runtime_error(strprintf("Invalid re-registration window start (%s)", vParams[0]));
    }
    if (!ParseInt32(vParams[1], &nPurgeHeight)) {
        throw std::runtime_error(strprintf("Invalid purge height (%s)", vParams[1]));
    }
    LogPrintf("Setting masternode purge parameters to windowStart=%ld, purgeHeight=%ld\n", nStartHeight, nPurgeHeight);
    UpdateMNPurgeParameters(nStartHeight, nPurgeHeight);
}

void CRegTestParams::UpdateLKSSmallQuorumHeightFromArgs(const ArgsManager& args)
{
    if (!args.IsArgSet("-lkssmallquorumheight")) return;

    int nActivationHeight;
    if (!ParseInt32(args.GetArg("-lkssmallquorumheight", ""), &nActivationHeight)) {
        throw std::runtime_error("Invalid activation height for -lkssmallquorumheight");
    }
    LogPrintf("Setting LKS small quorum activation height to %ld\n", nActivationHeight);
    UpdateLKSSmallQuorumHeight(nActivationHeight);
}

void CRegTestParams::UpdateBudgetParametersFromArgs(const ArgsManager& args)
{
    if (!args.IsArgSet("-budgetparams")) return;

    std::string strParams = args.GetArg("-budgetparams", "");
    std::vector<std::string> vParams;
    boost::split(vParams, strParams, boost::is_any_of(":"));
    if (vParams.size() != 3) {
        throw std::runtime_error("Budget parameters malformed, expecting <masternode>:<budget>:<superblock>");
    }
    int nMasternodePaymentsStartBlock, nBudgetPaymentsStartBlock, nSuperblockStartBlock;
    if (!ParseInt32(vParams[0], &nMasternodePaymentsStartBlock)) {
        throw std::runtime_error(strprintf("Invalid masternode start height (%s)", vParams[0]));
    }
    if (!ParseInt32(vParams[1], &nBudgetPaymentsStartBlock)) {
        throw std::runtime_error(strprintf("Invalid budget start block (%s)", vParams[1]));
    }
    if (!ParseInt32(vParams[2], &nSuperblockStartBlock)) {
        throw std::runtime_error(strprintf("Invalid superblock start height (%s)", vParams[2]));
    }
    LogPrintf("Setting budget parameters to masternode=%ld, budget=%ld, superblock=%ld\n", nMasternodePaymentsStartBlock, nBudgetPaymentsStartBlock, nSuperblockStartBlock);
    UpdateBudgetParameters(nMasternodePaymentsStartBlock, nBudgetPaymentsStartBlock, nSuperblockStartBlock);
}

void CRegTestParams::UpdateLLMQTestParametersFromArgs(const ArgsManager& args, const Consensus::LLMQType llmqType)
{
    assert(llmqType == Consensus::LLMQType::LLMQ_TEST || llmqType == Consensus::LLMQType::LLMQ_TEST_INSTANTSEND);

    std::string cmd_param{"-llmqtestparams"}, llmq_name{"LLMQ_TEST"};
    if (llmqType == Consensus::LLMQType::LLMQ_TEST_INSTANTSEND) {
        cmd_param = "-llmqtestinstantsendparams";
        llmq_name = "LLMQ_TEST_INSTANTSEND";
    }

    if (!args.IsArgSet(cmd_param)) return;

    std::string strParams = args.GetArg(cmd_param, "");
    std::vector<std::string> vParams;
    boost::split(vParams, strParams, boost::is_any_of(":"));
    if (vParams.size() != 2) {
        throw std::runtime_error(strprintf("%s parameters malformed, expecting <size>:<threshold>", llmq_name));
    }
    int size, threshold;
    if (!ParseInt32(vParams[0], &size)) {
        throw std::runtime_error(strprintf("Invalid %s size (%s)", llmq_name, vParams[0]));
    }
    if (!ParseInt32(vParams[1], &threshold)) {
        throw std::runtime_error(strprintf("Invalid %s threshold (%s)", llmq_name, vParams[1]));
    }
    LogPrintf("Setting %s parameters to size=%ld, threshold=%ld\n", llmq_name, size, threshold);
    UpdateLLMQTestParameters(size, threshold, llmqType);
}

void CDevNetParams::UpdateDevnetSubsidyAndDiffParametersFromArgs(const ArgsManager& args)
{
    if (!args.IsArgSet("-minimumdifficultyblocks") && !args.IsArgSet("-highsubsidyblocks") && !args.IsArgSet("-highsubsidyfactor")) return;

    int nMinimumDifficultyBlocks = gArgs.GetArg("-minimumdifficultyblocks", consensus.nMinimumDifficultyBlocks);
    int nHighSubsidyBlocks = gArgs.GetArg("-highsubsidyblocks", consensus.nHighSubsidyBlocks);
    int nHighSubsidyFactor = gArgs.GetArg("-highsubsidyfactor", consensus.nHighSubsidyFactor);
    LogPrintf("Setting minimumdifficultyblocks=%ld, highsubsidyblocks=%ld, highsubsidyfactor=%ld\n", nMinimumDifficultyBlocks, nHighSubsidyBlocks, nHighSubsidyFactor);
    UpdateDevnetSubsidyAndDiffParameters(nMinimumDifficultyBlocks, nHighSubsidyBlocks, nHighSubsidyFactor);
}

void CDevNetParams::UpdateDevnetLLMQChainLocksFromArgs(const ArgsManager& args)
{
    if (!args.IsArgSet("-llmqchainlocks")) return;

    const auto& llmq_params_opt = GetLLMQ(consensus.llmqTypeChainLocks);
    assert(llmq_params_opt.has_value());

    std::string strLLMQType = gArgs.GetArg("-llmqchainlocks", std::string(llmq_params_opt->name));

    Consensus::LLMQType llmqType = Consensus::LLMQType::LLMQ_NONE;
    for (const auto& params : consensus.llmqs) {
        if (params.name == strLLMQType) {
            if (params.useRotation) {
                throw std::runtime_error("LLMQ type specified for -llmqchainlocks must NOT use rotation");
            }
            llmqType = params.type;
        }
    }
    if (llmqType == Consensus::LLMQType::LLMQ_NONE) {
        throw std::runtime_error("Invalid LLMQ type specified for -llmqchainlocks.");
    }
    LogPrintf("Setting llmqchainlocks to size=%ld\n", static_cast<uint8_t>(llmqType));
    UpdateDevnetLLMQChainLocks(llmqType);
}

void CDevNetParams::UpdateDevnetLLMQInstantSendFromArgs(const ArgsManager& args)
{
    if (!args.IsArgSet("-llmqinstantsend")) return;

    const auto& llmq_params_opt = GetLLMQ(consensus.llmqTypeInstantSend);
    assert(llmq_params_opt.has_value());

    std::string strLLMQType = gArgs.GetArg("-llmqinstantsend", std::string(llmq_params_opt->name));

    Consensus::LLMQType llmqType = Consensus::LLMQType::LLMQ_NONE;
    for (const auto& params : consensus.llmqs) {
        if (params.name == strLLMQType) {
            if (params.useRotation) {
                throw std::runtime_error("LLMQ type specified for -llmqinstantsend must NOT use rotation");
            }
            llmqType = params.type;
        }
    }
    if (llmqType == Consensus::LLMQType::LLMQ_NONE) {
        throw std::runtime_error("Invalid LLMQ type specified for -llmqinstantsend.");
    }
    LogPrintf("Setting llmqinstantsend to size=%ld\n", static_cast<uint8_t>(llmqType));
    UpdateDevnetLLMQInstantSend(llmqType);
}

void CDevNetParams::UpdateDevnetLLMQInstantSendDIP0024FromArgs(const ArgsManager& args)
{
    if (!args.IsArgSet("-llmqinstantsenddip0024")) return;

    const auto& llmq_params_opt = GetLLMQ(consensus.llmqTypeDIP0024InstantSend);
    assert(llmq_params_opt.has_value());

    std::string strLLMQType = gArgs.GetArg("-llmqinstantsenddip0024", std::string(llmq_params_opt->name));

    Consensus::LLMQType llmqType = Consensus::LLMQType::LLMQ_NONE;
    for (const auto& params : consensus.llmqs) {
        if (params.name == strLLMQType) {
            if (!params.useRotation) {
                throw std::runtime_error("LLMQ type specified for -llmqinstantsenddip0024 must use rotation");
            }
            llmqType = params.type;
        }
    }
    if (llmqType == Consensus::LLMQType::LLMQ_NONE) {
        throw std::runtime_error("Invalid LLMQ type specified for -llmqinstantsenddip0024.");
    }
    LogPrintf("Setting llmqinstantsenddip0024 to size=%ld\n", static_cast<uint8_t>(llmqType));
    UpdateDevnetLLMQDIP0024InstantSend(llmqType);
}

void CDevNetParams::UpdateDevnetPowTargetSpacingFromArgs(const ArgsManager& args)
{
    if (!args.IsArgSet("-powtargetspacing")) return;

    std::string strPowTargetSpacing = gArgs.GetArg("-powtargetspacing", "");

    int64_t powTargetSpacing;
    if (!ParseInt64(strPowTargetSpacing, &powTargetSpacing)) {
        throw std::runtime_error(strprintf("Invalid parsing of powTargetSpacing (%s)", strPowTargetSpacing));
    }

    if (powTargetSpacing < 1) {
        throw std::runtime_error(strprintf("Invalid value of powTargetSpacing (%s)", strPowTargetSpacing));
    }

    LogPrintf("Setting powTargetSpacing to %ld\n", powTargetSpacing);
    UpdateDevnetPowTargetSpacing(powTargetSpacing);
}

void CDevNetParams::UpdateLLMQDevnetParametersFromArgs(const ArgsManager& args)
{
    if (!args.IsArgSet("-llmqdevnetparams")) return;

    std::string strParams = args.GetArg("-llmqdevnetparams", "");
    std::vector<std::string> vParams;
    boost::split(vParams, strParams, boost::is_any_of(":"));
    if (vParams.size() != 2) {
        throw std::runtime_error("LLMQ_DEVNET parameters malformed, expecting <size>:<threshold>");
    }
    int size, threshold;
    if (!ParseInt32(vParams[0], &size)) {
        throw std::runtime_error(strprintf("Invalid LLMQ_DEVNET size (%s)", vParams[0]));
    }
    if (!ParseInt32(vParams[1], &threshold)) {
        throw std::runtime_error(strprintf("Invalid LLMQ_DEVNET threshold (%s)", vParams[1]));
    }
    LogPrintf("Setting LLMQ_DEVNET parameters to size=%ld, threshold=%ld\n", size, threshold);
    UpdateLLMQDevnetParameters(size, threshold);
}

static std::unique_ptr<const CChainParams> globalChainParams;

const CChainParams &Params() {
    assert(globalChainParams);
    return *globalChainParams;
}

std::unique_ptr<const CChainParams> CreateChainParams(const std::string& chain)
{
    if (chain == CBaseChainParams::MAIN)
        return std::make_unique<CMainParams>();
    else if (chain == CBaseChainParams::TESTNET)
        return std::make_unique<CTestNetParams>();
    else if (chain == CBaseChainParams::DEVNET)
        return std::make_unique<CDevNetParams>(gArgs);
    else if (chain == CBaseChainParams::REGTEST)
        return std::make_unique<CRegTestParams>(gArgs);

    throw std::runtime_error(strprintf("%s: Unknown chain %s.", __func__, chain));
}

void SelectParams(const std::string& network)
{
    SelectBaseParams(network);
    globalChainParams = CreateChainParams(network);
}
