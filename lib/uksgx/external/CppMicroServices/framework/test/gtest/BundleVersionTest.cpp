/*=============================================================================

Library: CppMicroServices

Copyright (c) The CppMicroServices developers. See the COPYRIGHT
file at the top-level directory of this distribution and at
https://github.com/CppMicroServices/CppMicroServices/COPYRIGHT .

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

=============================================================================*/

#include "cppmicroservices/BundleVersion.h"
#include "gtest/gtest.h"

using cppmicroservices::BundleVersion;

TEST(BundleVersion, EmptyVersion)
{
  auto empty = BundleVersion::EmptyVersion();
  unsigned int zero(0);
  ASSERT_EQ(zero, empty.GetMajor());
  ASSERT_EQ(zero, empty.GetMinor());
  ASSERT_EQ(zero, empty.GetMicro());
  ASSERT_EQ(std::string(), empty.GetQualifier());
  ASSERT_FALSE(empty.IsUndefined());
}

TEST(BundleVersion, UndefinedVersion)
{
  auto undefined = BundleVersion::UndefinedVersion();
  ASSERT_THROW(undefined.GetMajor(), std::logic_error);
  ASSERT_THROW(undefined.GetMinor(), std::logic_error);
  ASSERT_THROW(undefined.GetMicro(), std::logic_error);
  ASSERT_THROW(undefined.GetQualifier(), std::logic_error);
  ASSERT_TRUE(undefined.IsUndefined());
}

TEST(BundleVersion, InvalidQualifier)
{
  ASSERT_THROW(
    {
      BundleVersion invalidQualifier(
        BundleVersion(1, 0, 0, std::string("<0>")));
    },
    std::invalid_argument);

  ASSERT_THROW(
    { BundleVersion invalidQualifier2(1, 0, 0, std::string("<?>0><")); },
    std::invalid_argument);

  ASSERT_NO_THROW(
    { BundleVersion validQualifier2(1, 0, 0, std::string("_aok_1")); });
}

TEST(BundleVersion, Ctor)
{
  BundleVersion onedotzero(1, 0, 0);
  ASSERT_FALSE(onedotzero.IsUndefined());

  BundleVersion copy_of_onedotzero(onedotzero);
  ASSERT_FALSE(copy_of_onedotzero.IsUndefined());
  ASSERT_EQ(onedotzero.GetMajor(), copy_of_onedotzero.GetMajor());
  ASSERT_EQ(onedotzero.GetMinor(), copy_of_onedotzero.GetMinor());
  ASSERT_EQ(onedotzero.GetMicro(), copy_of_onedotzero.GetMicro());

  ASSERT_NO_THROW(BundleVersion validQualifier(1, 0, 0, std::string("_a01")));
  ASSERT_NO_THROW(BundleVersion validQualifier2(1, 0, 0, std::string("-a01-")));

  ASSERT_NO_THROW(BundleVersion stringVersion(std::string("0.0.1_qa0_")));

  ASSERT_THROW(
    BundleVersion invalidVersion(std::string("eight.two.twenty._not_ok")),
    std::invalid_argument);

  ASSERT_THROW(
    BundleVersion tooManyVersions(std::string("1.1.1._ok.extra_version")),
    std::invalid_argument);
}

TEST(BundleVersion, Accessors)
{
  unsigned int major = 8;
  unsigned int minor = 1;
  unsigned int micro = 999999;
  BundleVersion ver(major, minor, micro, std::string("_a_ok1010"));

  ASSERT_EQ(major, ver.GetMajor());
  ASSERT_EQ(minor, ver.GetMinor());
  ASSERT_EQ(micro, ver.GetMicro());
  ASSERT_EQ(std::string("_a_ok1010"), ver.GetQualifier());
}

TEST(BundleVersion, ToString)
{
  ASSERT_EQ(std::string("undefined"),
            BundleVersion::UndefinedVersion().ToString());
  ASSERT_EQ(std::string("0.0.0"), BundleVersion::EmptyVersion().ToString());

  ASSERT_EQ(std::string("1.0.0"), BundleVersion(1, 0, 0).ToString());
  ASSERT_EQ(std::string("1.0.0._qualifier1"),
            BundleVersion(1, 0, 0, std::string("_qualifier1")).ToString());
}

TEST(BundleVersion, Comparison)
{
  BundleVersion zeroVersion(0, 0, 0);
  BundleVersion alphaVersion(0, 0, 1);
  BundleVersion betaVersion(0, 1, 0);
  BundleVersion releaseVersion(1, 0, 0);

  ASSERT_THROW(zeroVersion.Compare(BundleVersion::UndefinedVersion()),
               std::logic_error);
  ASSERT_THROW(BundleVersion::UndefinedVersion().Compare(zeroVersion),
               std::logic_error);

  ASSERT_EQ(0, zeroVersion.Compare(zeroVersion));
  ASSERT_NE(0, zeroVersion.Compare(alphaVersion));

  ASSERT_EQ(-1, zeroVersion.Compare(alphaVersion));
  ASSERT_EQ(0, zeroVersion.Compare(BundleVersion::EmptyVersion()));
  ASSERT_EQ(1, betaVersion.Compare(alphaVersion));
  ASSERT_EQ(-1, alphaVersion.Compare(betaVersion));
  ASSERT_EQ(-1, betaVersion.Compare(releaseVersion));

  ASSERT_FALSE(zeroVersion == BundleVersion::UndefinedVersion());
  ASSERT_TRUE(zeroVersion == zeroVersion);
  ASSERT_TRUE(zeroVersion == BundleVersion::EmptyVersion());
  ASSERT_FALSE(zeroVersion == alphaVersion);
  ASSERT_FALSE(alphaVersion == betaVersion);
}

TEST(BundleVersion, ParseVersion)
{
  ASSERT_TRUE(BundleVersion::EmptyVersion() ==
              BundleVersion::ParseVersion(std::string()));
  ASSERT_TRUE(BundleVersion::EmptyVersion() ==
              BundleVersion::ParseVersion(std::string("           ")));
  ASSERT_FALSE(BundleVersion::EmptyVersion() ==
               BundleVersion::ParseVersion(std::string("     1     ")));

  ASSERT_NO_THROW(BundleVersion::ParseVersion(std::string("0.0.1")));
  ASSERT_NO_THROW(BundleVersion::ParseVersion(std::string("0.0.1_abc123")));
  ASSERT_NO_THROW(BundleVersion::ParseVersion(std::string("0.0.1-xyz098")));

  // FIXME: THIS SHOULD THROW OR FAIL SOMEHOW --> ASSERT_THROW(BundleVersion::ParseVersion(std::string("0.0.1-??0??")), std::invalid_argument);
  ASSERT_THROW(BundleVersion::ParseVersion(std::string("0.0.1.-??0??")),
               std::invalid_argument);
}
