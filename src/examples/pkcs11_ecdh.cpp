#include <botan/asn1_obj.h>
#include <botan/auto_rng.h>
#include <botan/der_enc.h>
#include <botan/ec_group.h>
#include <botan/ecdh.h>
#include <botan/p11.h>
#include <botan/p11_ecc_key.h>
#include <botan/p11_ecdh.h>
#include <botan/pubkey.h>
#include <botan/symkey.h>

#include <string>

int main()
   {
   Botan::PKCS11::secure_string pin = { '1', '2', '3', '4', '5', '6' };
   session.login( Botan::PKCS11::UserType::User, pin );

   /************ import ECDH private key *************/

   Botan::AutoSeeded_RNG rng;

   // create private key in software
   Botan::ECDH_PrivateKey priv_key_sw( rng, Botan::EC_Group( "secp256r1" ) );
   priv_key_sw.set_parameter_encoding( Botan::EC_Group_Encoding::EC_DOMPAR_ENC_OID );

   // set import properties
   Botan::PKCS11::EC_PrivateKeyImportProperties priv_import_props(
      priv_key_sw.DER_domain(), priv_key_sw.private_value() );

   priv_import_props.set_token( true );
   priv_import_props.set_private( true );
   priv_import_props.set_derive( true );
   priv_import_props.set_extractable( true );

   // label
   std::string label = "test ECDH key";
   priv_import_props.set_label( label );

   // import to card
   Botan::PKCS11::PKCS11_ECDH_PrivateKey priv_key( session, priv_import_props );

   /************ export ECDH private key *************/
   Botan::ECDH_PrivateKey exported = priv_key.export_key();

   /************ import ECDH public key *************/

   // set import properties
   Botan::PKCS11::EC_PublicKeyImportProperties pub_import_props( priv_key_sw.DER_domain(),
      Botan::DER_Encoder().encode(
         priv_key_sw.public_point().encode(Botan::PointGFp::Compression_Type::UNCOMPRESSED ),
         Botan::ASN1_Tag::OCTET_STRING ).get_contents_unlocked() );

   pub_import_props.set_token( true );
   pub_import_props.set_private( false );
   pub_import_props.set_derive( true );

   // label
   label = "test ECDH pub key";
   pub_import_props.set_label( label );

   // import
   Botan::PKCS11::PKCS11_ECDH_PublicKey pub_key( session, pub_import_props );

   /************ export ECDH private key *************/
   Botan::ECDH_PublicKey exported_pub = pub_key.export_key();

   /************ generate ECDH private key *************/

   Botan::PKCS11::EC_PrivateKeyGenerationProperties priv_generate_props;
   priv_generate_props.set_token( true );
   priv_generate_props.set_private( true );
   priv_generate_props.set_derive( true );

   Botan::PKCS11::PKCS11_ECDH_PrivateKey priv_key2( session,
      Botan::EC_Group( "secp256r1" ).DER_encode( Botan::EC_Group_Encoding::EC_DOMPAR_ENC_OID ),
      priv_generate_props );

   /************ generate ECDH key pair *************/

   Botan::PKCS11::EC_PublicKeyGenerationProperties pub_generate_props(
      Botan::EC_Group( "secp256r1" ).DER_encode( Botan::EC_Group_Encoding::EC_DOMPAR_ENC_OID ) );

   pub_generate_props.set_label( label + "_PUB_KEY" );
   pub_generate_props.set_token( true );
   pub_generate_props.set_derive( true );
   pub_generate_props.set_private( false );
   pub_generate_props.set_modifiable( true );

   Botan::PKCS11::PKCS11_ECDH_KeyPair key_pair = Botan::PKCS11::generate_ecdh_keypair(
      session, pub_generate_props, priv_generate_props );

   /************ ECDH derive *************/

   Botan::PKCS11::PKCS11_ECDH_KeyPair key_pair_other = Botan::PKCS11::generate_ecdh_keypair(
      session, pub_generate_props, priv_generate_props );

   Botan::PK_Key_Agreement ka( key_pair.second, rng, "Raw", "pkcs11" );
   Botan::PK_Key_Agreement kb( key_pair_other.second, rng, "Raw", "pkcs11" );

   Botan::SymmetricKey alice_key = ka.derive_key( 32,
      key_pair_other.first.public_point().encode(Botan::PointGFp::UNCOMPRESSED));

   Botan::SymmetricKey bob_key = kb.derive_key( 32,
      key_pair.first.public_point().encode(Botan::PointGFp::UNCOMPRESSED));

   bool eq = alice_key == bob_key;
   }
