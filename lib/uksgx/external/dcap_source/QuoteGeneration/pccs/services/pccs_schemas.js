/*
 * Copyright (C) 2011-2021 Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Intel Corporation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
export const PLATFORM_REG_SCHEMA = {
  title: 'Platform Registration',
  description: 'Platform Registration Data Format',
  type: 'object',
  properties: {
    qe_id: {
      type: 'string',
      minLength: 1,
      maxLength: 260,
    },
    pce_id: {
      type: 'string',
      pattern: '^[a-fA-F0-9]{4}$',
    },
    cpu_svn: {
      type: 'string',
      pattern: '^[a-fA-F0-9]{32}$',
    },
    pce_svn: {
      type: 'string',
      pattern: '^[a-fA-F0-9]{4}$',
    },
    enc_ppid: {
      type: 'string',
      pattern: '^[a-fA-F0-9]{768}$',
    },
    platform_manifest: {
      type: 'string',
    },
  },
  required: ['qe_id', 'pce_id'],
};

export const PLATFORM_COLLATERAL_SCHEMA_V3 = {
  title: 'Platform Registration',
  description: 'Platform Registration Data Format',
  type: 'object',
  properties: {
    platforms: {
      type: 'array',
      items: {
        'type:': 'object',
        properties: {
          qe_id: {
            type: 'string',
            minLength: 1,
            maxLength: 260,
          },
          pce_id: {
            type: 'string',
            pattern: '^[a-fA-F0-9]{4}$',
          },
          cpu_svn: {
            type: 'string',
            pattern: '^[a-fA-F0-9]{32}$|^$',
          },
          pce_svn: {
            type: 'string',
            pattern: '^[a-fA-F0-9]{4}$|^$',
          },
          enc_ppid: {
            type: 'string',
            pattern: '^[a-fA-F0-9]{768}$|^$',
          },
          platform_manifest: {
            type: 'string',
          },
        },
        required: ['qe_id', 'pce_id'],
      },
    },
    collaterals: {
      type: 'object',
      properties: {
        pck_certs: {
          type: 'array',
          items: {
            type: 'object',
            properties: {
              qe_id: {
                type: 'string',
                minLength: 1,
                maxLength: 260,
              },
              pce_id: {
                type: 'string',
                pattern: '^[a-fA-F0-9]{4}$',
              },
              enc_ppid: {
                type: 'string',
                pattern: '^[a-fA-F0-9]{768}$|^$',
              },
              platform_manifest: {
                type: 'string',
              },
              certs: {
                type: 'array',
                items: {
                  type: 'object',
                  properties: {
                    tcb: {
                      type: 'object',
                      properties: {
                        sgxtcbcomp01svn: {
                          type: 'integer',
                          minimum: 0,
                          maximum: 255,
                        },
                        sgxtcbcomp02svn: {
                          type: 'integer',
                          minimum: 0,
                          maximum: 255,
                        },
                        sgxtcbcomp03svn: {
                          type: 'integer',
                          minimum: 0,
                          maximum: 255,
                        },
                        sgxtcbcomp04svn: {
                          type: 'integer',
                          minimum: 0,
                          maximum: 255,
                        },
                        sgxtcbcomp05svn: {
                          type: 'integer',
                          minimum: 0,
                          maximum: 255,
                        },
                        sgxtcbcomp06svn: {
                          type: 'integer',
                          minimum: 0,
                          maximum: 255,
                        },
                        sgxtcbcomp07svn: {
                          type: 'integer',
                          minimum: 0,
                          maximum: 255,
                        },
                        sgxtcbcomp08svn: {
                          type: 'integer',
                          minimum: 0,
                          maximum: 255,
                        },
                        sgxtcbcomp09svn: {
                          type: 'integer',
                          minimum: 0,
                          maximum: 255,
                        },
                        sgxtcbcomp10svn: {
                          type: 'integer',
                          minimum: 0,
                          maximum: 255,
                        },
                        sgxtcbcomp11svn: {
                          type: 'integer',
                          minimum: 0,
                          maximum: 255,
                        },
                        sgxtcbcomp12svn: {
                          type: 'integer',
                          minimum: 0,
                          maximum: 255,
                        },
                        sgxtcbcomp13svn: {
                          type: 'integer',
                          minimum: 0,
                          maximum: 255,
                        },
                        sgxtcbcomp14svn: {
                          type: 'integer',
                          minimum: 0,
                          maximum: 255,
                        },
                        sgxtcbcomp15svn: {
                          type: 'integer',
                          minimum: 0,
                          maximum: 255,
                        },
                        sgxtcbcomp16svn: {
                          type: 'integer',
                          minimum: 0,
                          maximum: 255,
                        },
                        pcesvn: {
                          type: 'integer',
                          minimum: 0,
                          maximum: 65535,
                        },
                      },
                    },
                    tcbm: {
                      type: 'string',
                      pattern: '^[0-9a-fA-F]{36}$',
                    },
                    cert: {
                      type: 'string',
                    },
                  },
                  required: ['tcb', 'tcbm', 'cert'],
                },
              },
            },
            required: ['qe_id', 'pce_id', 'enc_ppid', 'certs'],
          },
        },
        tcbinfos: {
          type: 'array',
          items: {
            type: 'object',
            properties: {
              fmspc: {
                type: 'string',
              },
              tcbinfo: {
                type: 'object',
                properties: {
                  tcbInfo: {
                    type: 'object',
                    properties: {
                      version: {
                        type: 'integer',
                      },
                      issueDate: {
                        type: 'string',
                        format: 'date-time',
                      },
                      nextUpdate: {
                        type: 'string',
                        format: 'date-time',
                      },
                      fmspc: {
                        type: 'string',
                        pattern: '^[0-9a-fA-F]{12}$',
                      },
                      pceId: {
                        type: 'string',
                        pattern: '^[0-9a-fA-F]{4}$',
                      },
                      tcbType: {
                        type: 'integer',
                      },
                      tcbEvaluationDataNumber: {
                        type: 'integer',
                      },
                      tcbLevels: {
                        type: 'array',
                        items: {
                          type: 'object',
                          properties: {
                            tcb: {
                              type: 'object',
                              properties: {
                                pcesvn: {
                                  type: 'integer',
                                },
                                sgxtcbcomp01svn: {
                                  type: 'integer',
                                  minimum: 0,
                                  maximum: 255,
                                },
                                sgxtcbcomp02svn: {
                                  type: 'integer',
                                  minimum: 0,
                                  maximum: 255,
                                },
                                sgxtcbcomp03svn: {
                                  type: 'integer',
                                  minimum: 0,
                                  maximum: 255,
                                },
                                sgxtcbcomp04svn: {
                                  type: 'integer',
                                  minimum: 0,
                                  maximum: 255,
                                },
                                sgxtcbcomp05svn: {
                                  type: 'integer',
                                  minimum: 0,
                                  maximum: 255,
                                },
                                sgxtcbcomp06svn: {
                                  type: 'integer',
                                  minimum: 0,
                                  maximum: 255,
                                },
                                sgxtcbcomp07svn: {
                                  type: 'integer',
                                  minimum: 0,
                                  maximum: 255,
                                },
                                sgxtcbcomp08svn: {
                                  type: 'integer',
                                  minimum: 0,
                                  maximum: 255,
                                },
                                sgxtcbcomp09svn: {
                                  type: 'integer',
                                  minimum: 0,
                                  maximum: 255,
                                },
                                sgxtcbcomp10svn: {
                                  type: 'integer',
                                  minimum: 0,
                                  maximum: 255,
                                },
                                sgxtcbcomp11svn: {
                                  type: 'integer',
                                  minimum: 0,
                                  maximum: 255,
                                },
                                sgxtcbcomp12svn: {
                                  type: 'integer',
                                  minimum: 0,
                                  maximum: 255,
                                },
                                sgxtcbcomp13svn: {
                                  type: 'integer',
                                  minimum: 0,
                                  maximum: 255,
                                },
                                sgxtcbcomp14svn: {
                                  type: 'integer',
                                  minimum: 0,
                                  maximum: 255,
                                },
                                sgxtcbcomp15svn: {
                                  type: 'integer',
                                  minimum: 0,
                                  maximum: 255,
                                },
                                sgxtcbcomp16svn: {
                                  type: 'integer',
                                  minimum: 0,
                                  maximum: 255,
                                },
                              },
                            },
                            tcbDate: {
                              type: 'string',
                              format: 'date-time',
                            },
                            tcbStatus: {
                              type: 'string',
                            },
                            advisoryIDs: {
                              type: 'array',
                              items: {
                                type: 'string',
                              },
                            },
                          },
                        },
                      },
                    },
                  },
                  signature: {
                    type: 'string',
                  },
                },
                required: ['tcbInfo', 'signature'],
              },
            },
            required: ['fmspc', 'tcbinfo'],
          },
        },
        pckcacrl: {
          type: 'object',
          properties: {
            processorCrl: {
              type: 'string',
            },
            platformCrl: {
              type: 'string',
            },
          },
        },
        qeidentity: {
          type: 'string',
        },
        qveidentity: {
          type: 'string',
        },
        certificates: {
          type: 'object',
          properties: {
            'SGX-PCK-Certificate-Issuer-Chain': {
              type: 'object',
              properties: {
                PROCESSOR: {
                  type: 'string',
                },
                PLATFORM: {
                  type: 'string',
                },
              },
            },
            'SGX-TCB-Info-Issuer-Chain': {
              type: 'string',
            },
            'SGX-Enclave-Identity-Issuer-Chain': {
              type: 'string',
            },
          },
          required: ['SGX-PCK-Certificate-Issuer-Chain'],
        },
        rootcacrl: {
          type: 'string',
        },
      },
      required: ['pck_certs', 'tcbinfos', 'certificates'],
    },
  },
  required: ['platforms', 'collaterals'],
};

export const PLATFORM_COLLATERAL_SCHEMA_V4 = {
  title: 'Platform Registration',
  description: 'Platform Registration Data Format',
  type: 'object',
  properties: {
    platforms: {
      type: 'array',
      items: {
        'type:': 'object',
        properties: {
          qe_id: {
            type: 'string',
            minLength: 1,
            maxLength: 260,
          },
          pce_id: {
            type: 'string',
            pattern: '^[a-fA-F0-9]{4}$',
          },
          cpu_svn: {
            type: 'string',
            pattern: '^[a-fA-F0-9]{32}$|^$',
          },
          pce_svn: {
            type: 'string',
            pattern: '^[a-fA-F0-9]{4}$|^$',
          },
          enc_ppid: {
            type: 'string',
            pattern: '^[a-fA-F0-9]{768}$|^$',
          },
          platform_manifest: {
            type: 'string',
          },
        },
        required: ['qe_id', 'pce_id'],
      },
    },
    collaterals: {
      type: 'object',
      properties: {
        pck_certs: {
          type: 'array',
          items: {
            type: 'object',
            properties: {
              qe_id: {
                type: 'string',
                minLength: 1,
                maxLength: 260,
              },
              pce_id: {
                type: 'string',
                pattern: '^[a-fA-F0-9]{4}$',
              },
              enc_ppid: {
                type: 'string',
                pattern: '^[a-fA-F0-9]{768}$|^$',
              },
              platform_manifest: {
                type: 'string',
              },
              certs: {
                type: 'array',
                items: {
                  type: 'object',
                  properties: {
                    tcb: {
                      type: 'object',
                      properties: {
                        sgxtcbcomp01svn: {
                          type: 'integer',
                          minimum: 0,
                          maximum: 255,
                        },
                        sgxtcbcomp02svn: {
                          type: 'integer',
                          minimum: 0,
                          maximum: 255,
                        },
                        sgxtcbcomp03svn: {
                          type: 'integer',
                          minimum: 0,
                          maximum: 255,
                        },
                        sgxtcbcomp04svn: {
                          type: 'integer',
                          minimum: 0,
                          maximum: 255,
                        },
                        sgxtcbcomp05svn: {
                          type: 'integer',
                          minimum: 0,
                          maximum: 255,
                        },
                        sgxtcbcomp06svn: {
                          type: 'integer',
                          minimum: 0,
                          maximum: 255,
                        },
                        sgxtcbcomp07svn: {
                          type: 'integer',
                          minimum: 0,
                          maximum: 255,
                        },
                        sgxtcbcomp08svn: {
                          type: 'integer',
                          minimum: 0,
                          maximum: 255,
                        },
                        sgxtcbcomp09svn: {
                          type: 'integer',
                          minimum: 0,
                          maximum: 255,
                        },
                        sgxtcbcomp10svn: {
                          type: 'integer',
                          minimum: 0,
                          maximum: 255,
                        },
                        sgxtcbcomp11svn: {
                          type: 'integer',
                          minimum: 0,
                          maximum: 255,
                        },
                        sgxtcbcomp12svn: {
                          type: 'integer',
                          minimum: 0,
                          maximum: 255,
                        },
                        sgxtcbcomp13svn: {
                          type: 'integer',
                          minimum: 0,
                          maximum: 255,
                        },
                        sgxtcbcomp14svn: {
                          type: 'integer',
                          minimum: 0,
                          maximum: 255,
                        },
                        sgxtcbcomp15svn: {
                          type: 'integer',
                          minimum: 0,
                          maximum: 255,
                        },
                        sgxtcbcomp16svn: {
                          type: 'integer',
                          minimum: 0,
                          maximum: 255,
                        },
                        pcesvn: {
                          type: 'integer',
                          minimum: 0,
                          maximum: 65535,
                        },
                      },
                    },
                    tcbm: {
                      type: 'string',
                      pattern: '^[0-9a-fA-F]{36}$',
                    },
                    cert: {
                      type: 'string',
                    },
                  },
                  required: ['tcb', 'tcbm', 'cert'],
                },
              },
            },
            required: ['qe_id', 'pce_id', 'enc_ppid', 'certs'],
          },
        },
        tcbinfos: {
          type: 'array',
          items: {
            type: 'object',
            properties: {
              fmspc: {
                type: 'string',
              },
              sgx_tcbinfo: {
                type: 'object',
                properties: {
                  tcbInfo: {
                    type: 'object',
                    properties: {
                      id: {
                        const: 'SGX',
                      },
                      version: {
                        type: 'integer',
                      },
                      issueDate: {
                        type: 'string',
                        format: 'date-time',
                      },
                      nextUpdate: {
                        type: 'string',
                        format: 'date-time',
                      },
                      fmspc: {
                        type: 'string',
                        pattern: '^[0-9a-fA-F]{12}$',
                      },
                      pceId: {
                        type: 'string',
                        pattern: '^[0-9a-fA-F]{4}$',
                      },
                      tcbType: {
                        type: 'integer',
                      },
                      tcbEvaluationDataNumber: {
                        type: 'integer',
                      },
                      tcbLevels: {
                        type: 'array',
                        items: {
                          type: 'object',
                          properties: {
                            tcb: {
                              type: 'object',
                              properties: {
                                sgxtcbcomponents: {
                                  type: 'array',
                                  items: {
                                    type: 'object',
                                    properties: {
                                      svn: {
                                        type: 'integer',
                                      },
                                      category: {
                                        type: 'string',
                                      },
                                      type: {
                                        type: 'string',
                                      },
                                    },
                                    required: ['svn'],
                                  },
                                },
                                pcesvn: {
                                  type: 'integer',
                                },
                              },
                              required: ['sgxtcbcomponents'],
                            },
                            tcbDate: {
                              type: 'string',
                              format: 'date-time',
                            },
                            tcbStatus: {
                              type: 'string',
                            },
                            advisoryIDs: {
                              type: 'array',
                              items: {
                                type: 'string',
                              },
                            },
                          },
                        },
                      },
                    },
                  },
                  signature: {
                    type: 'string',
                  },
                },
                required: ['tcbInfo', 'signature'],
              },
              tdx_tcbinfo: {
                type: 'object',
                properties: {
                  tcbInfo: {
                    type: 'object',
                    properties: {
                      id: {
                        const: 'TDX',
                      },
                      version: {
                        type: 'integer',
                      },
                      issueDate: {
                        type: 'string',
                        format: 'date-time',
                      },
                      nextUpdate: {
                        type: 'string',
                        format: 'date-time',
                      },
                      fmspc: {
                        type: 'string',
                        pattern: '^[0-9a-fA-F]{12}$',
                      },
                      pceId: {
                        type: 'string',
                        pattern: '^[0-9a-fA-F]{4}$',
                      },
                      tcbType: {
                        type: 'integer',
                      },
                      tcbEvaluationDataNumber: {
                        type: 'integer',
                      },
                      mrsignerseam: {
                        type: 'string',
                      },
                      tcbLevels: {
                        type: 'array',
                        items: {
                          type: 'object',
                          properties: {
                            tcb: {
                              type: 'object',
                              properties: {
                                sgxtcbcomponents: {
                                  type: 'array',
                                  items: {
                                    type: 'object',
                                    properties: {
                                      svn: {
                                        type: 'integer',
                                      },
                                      category: {
                                        type: 'string',
                                      },
                                      type: {
                                        type: 'string',
                                      },
                                    },
                                    required: ['svn'],
                                  },
                                },
                                pcesvn: {
                                  type: 'integer',
                                },
                                tdxtcbcomponents: {
                                  type: 'array',
                                  items: {
                                    type: 'object',
                                    properties: {
                                      svn: {
                                        type: 'integer',
                                      },
                                      category: {
                                        type: 'string',
                                      },
                                      type: {
                                        type: 'string',
                                      },
                                    },
                                    required: ['svn'],
                                  },
                                },
                              },
                              required: ['sgxtcbcomponents'],
                            },
                            tcbDate: {
                              type: 'string',
                              format: 'date-time',
                            },
                            tcbStatus: {
                              type: 'string',
                            },
                            advisoryIDs: {
                              type: 'array',
                              items: {
                                type: 'string',
                              },
                            },
                          },
                        },
                      },
                    },
                  },
                  signature: {
                    type: 'string',
                  },
                },
                required: ['tcbInfo', 'signature'],
              },
            },
            required: ['fmspc'],
          },
        },
        pckcacrl: {
          type: 'object',
          properties: {
            processorCrl: {
              type: 'string',
            },
            platformCrl: {
              type: 'string',
            },
          },
        },
        qeidentity: {
          type: 'string',
        },
        tdqeidentity: {
          type: 'string',
        },
        qveidentity: {
          type: 'string',
        },
        certificates: {
          type: 'object',
          properties: {
            'SGX-PCK-Certificate-Issuer-Chain': {
              type: 'object',
              properties: {
                PROCESSOR: {
                  type: 'string',
                },
                PLATFORM: {
                  type: 'string',
                },
              },
            },
            'SGX-TCB-Info-Issuer-Chain': {
              type: 'string',
            },
            'SGX-Enclave-Identity-Issuer-Chain': {
              type: 'string',
            },
          },
          required: ['SGX-PCK-Certificate-Issuer-Chain'],
        },
        rootcacrl: {
          type: 'string',
        },
      },
      required: ['pck_certs', 'tcbinfos', 'certificates'],
    },
  },
  required: ['platforms', 'collaterals'],
};
