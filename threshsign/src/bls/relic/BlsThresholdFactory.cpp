// Concord
//
// Copyright (c) 2018 VMware, Inc. All Rights Reserved.
//
// This product is licensed to you under the Apache 2.0 license (the "License").
// You may not use this product except in compliance with the Apache 2.0 License.
//
// This product may include a number of subcomponents with separate copyright
// notices and license terms. Your use of these subcomponents is subject to the
// terms and conditions of the subcomponent's license, as noted in the
// LICENSE file.

#ifdef ERROR // TODO(GG): should be fixed by encapsulating relic (or windows) definitions in cpp files
#undef ERROR
#endif

#include "threshsign/Configuration.h"

#include <vector>
#include <iterator>
#include <algorithm>
#include <typeinfo>

#include "threshsign/bls/relic/BlsThresholdFactory.h"
#include "threshsign/bls/relic/Library.h"
#include "threshsign/bls/relic/BlsMultisigAccumulator.h"
#include "threshsign/bls/relic/BlsThresholdVerifier.h"
#include "threshsign/bls/relic/BlsMultisigVerifier.h"
#include "threshsign/bls/relic/BlsThresholdSigner.h"
#include "threshsign/bls/relic/BlsMultisigKeygen.h"
#include "threshsign/bls/relic/BlsPublicParameters.h"
#include "threshsign/bls/relic/BlsPublicKey.h"
#include "threshsign/bls/relic/PublicParametersFactory.h"

#include "BlsPolynomial.h"

#include "Log.h"

using std::endl;

namespace BLS {
namespace Relic {

BlsThresholdFactory::BlsThresholdFactory(const BlsPublicParameters& params)
    : params(params)
{
    if(params.getCurveType() != BLS::Relic::Library::Get().getCurrentCurve()) {
        throw std::runtime_error("Unsupported curve type");
    }
}

std::unique_ptr<BlsThresholdKeygenBase> BlsThresholdFactory::newKeygen(NumSharesType reqSigners, NumSharesType totalSigners) const {
    std::unique_ptr<BlsThresholdKeygenBase> keygen;
    if(reqSigners != totalSigners) {
        keygen.reset(new BlsThresholdKeygen(params, reqSigners, totalSigners));
    } else {
        keygen.reset(new BlsMultisigKeygen(params, totalSigners));
    }
    return keygen;
}

IThresholdVerifier * BlsThresholdFactory::newVerifier(NumSharesType reqSigners, NumSharesType totalSigners,
        const char * publicKeyStr, const std::vector<std::string>& verifKeysStr) const {
    G2T pk( (std::string(publicKeyStr)) );

    std::vector<BlsPublicKey> verifKeys;
    verifKeys.push_back(BlsPublicKey());	// signer 0 has no PK

    // Getting fancy now: converting strings to PKs!
    auto begin = verifKeysStr.begin();
    begin++;    // have to skip over signer 0, which doesn't exist
    std::transform(begin, verifKeysStr.end(), std::back_inserter(verifKeys),
            [](const std::string& str) -> BlsPublicKey { return BlsPublicKey(G2T(str)); });

    if(reqSigners == totalSigners)
        return new BlsMultisigVerifier(params, pk, totalSigners, verifKeys);
    else
        return new BlsThresholdVerifier(params, pk, reqSigners, totalSigners, verifKeys);
}

IThresholdSigner * BlsThresholdFactory::newSigner(ShareID id, const char * secretKeyStr) const {
    return new BlsThresholdSigner(params, id, BNT(std::string(secretKeyStr)));
}

std::tuple<std::vector<IThresholdSigner*>, IThresholdVerifier*> BlsThresholdFactory::newRandomSigners(
        NumSharesType reqSigners, NumSharesType numSigners) const {
    std::unique_ptr<BlsThresholdKeygenBase> keygen(newKeygen(reqSigners, numSigners));

    // Create signers
    std::vector<IThresholdSigner*> sks(static_cast<size_t>(numSigners + 1)); // 1-based indices
    std::vector<BlsPublicKey> verifKeys; // 1-based indices as well
    verifKeys.push_back(BlsPublicKey());	// signer 0 has no PK

    for (ShareID i = 1; i <= numSigners; i++) {
        size_t idx = static_cast<size_t>(i);    // thanks, C++!

        sks[idx] = new BlsThresholdSigner(params, i, keygen->getShareSecretKey(i));
        verifKeys.push_back(dynamic_cast<const BlsPublicKey&>(sks[idx]->getShareVerificationKey()));
    }

    // Create verifier
    IThresholdVerifier* verifier;
    if(reqSigners == numSigners) {
        logdbg << "Creating multisig BLS verifier" << endl;
        verifier = new BlsMultisigVerifier(params, keygen->getPublicKey(), numSigners, verifKeys);
    } else {
        verifier = new BlsThresholdVerifier(params, keygen->getPublicKey(), reqSigners, numSigners,
                verifKeys);
    }

    return std::make_tuple(sks, verifier);
}

} /* namespace Relic */
} /* namespace BLS */
